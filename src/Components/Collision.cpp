#include "Collision.h"

#include <cmath>

#include "Components/PlayerControl.h"
#include "Components/RigidBody.h"
#include "Components/Tags.h"
#include "Components/Transform.h"
#include "Components/TriggerZone.h"
#include "Components/Velocity.h"

#include "IGame.h"
#include "Map/Level.h"
#include "Systems/ColliderSystem.h"

#include "Events/Events.h"
#include "Physics/CollisionLayer.h"
#include "Physics/CollisionUtil.h"
#include "Physics/HitInfo.h"
#include "Physics/MaterialData.h"

#include "Sys/System.h"

#include "Util/JsonUtil.h"
#include "Util/MathUtil.h"

namespace whal {

constexpr s32 MOMENTUM_LIFETIME_FRAMES = 10;
constexpr s32 MOMENTUM_COOLDOWN_FRAMES = 10;
constexpr s32 BOUNCE_THRESHOLD = 2;  // need to be moving at least 2px/sec to bounce

void defaultSquish(ecs::Entity callbackEntity, ecs::Entity other, Vector2i hitNormal) {
    auto& callbackEntityCollider = callbackEntity.get<Collider>();
    if (callbackEntityCollider.isSemiSolid() && other.get<Collider>().isSemiSolid()) {
        return;
    }
    callbackEntityCollider.getEntity().kill();
    callbackEntityCollider.setIsDead();
}

// try wiggling out of upward collision.
bool defaultWiggle(Wiggle wiggleComponent, Collider& callbackCollider, HitInfo hitinfo, Vector2i moveNormal, Vector2f fullMoveAmount) {
    auto nextPos = callbackCollider.getShape().getPosition() + moveNormal;
    if ((hitinfo.otherLayer & (callbackCollider.getCollisionLayersThatCanStopMe())) > 0) {
        return callbackCollider.tryCornerCorrection(nextPos, fullMoveAmount.x, moveNormal, wiggleComponent.wiggleAmount);
    }
    return false;
}

void squishEntity(ecs::Entity callbackEntity, ecs::Entity other, Vector2i hitNormal) {
    callbackEntity.get<Collider>().squish(other, hitNormal);
}

// a semisolid pushing another semisolid shouldn't squish it. If it runs into a [semi]solid, just stop movement
void squishEntityPushedBySemiSolid(ecs::Entity callbackEntity, ecs::Entity other, Vector2i hitNormal) {
    auto& callbackEntityCollider = callbackEntity.get<Collider>();
    if (callbackEntityCollider.isSemiSolid() && other.get<Collider>().isSolidAny()) {
        return;
    }
    callbackEntityCollider.squish(other, hitNormal);
}

Collider::Collider(Transform transform, Vector2i halflen, CollisionLayer::Layer layer, ColliderParams params)
    : mShape(AABB(transform, halflen, params.offset)), mOffset(params.offset), mCollisionLayer(layer), mOnCollisionEnter(params.onCollisionEnter),
      mSquishCallback(params.onSquish), mMaterial(params.material), mCollisionDir(params.collisionDir), mCollisionMask(layer) {}

void Collider::setCollisionCallback(CollisionCallback callback) {
    mOnCollisionEnter = callback;
}

void Collider::setEntity(ecs::Entity entity) {
    mSelf = entity;
}

// syncs other engine components (Transform, and Trigger) with collider position
void Collider::updateEntityPosition() {
    Transform& trans = mSelf.get<Transform>();
    auto const shape = getShape();
    Vector2f newPosition = trans.applyInverse(shape.getPosition().as<f32>(), getOffset().as<f32>());
    const Vector2i newPositionInt = newPosition.round();

    // make sure player(s) can't go out of bounds
    if (mSelf.has<Player>()) {
        if (System::getGame().getScene().getLevelAt(newPositionInt)) {
            trans.setPosition(newPosition, mSelf);
        } else {
            // tried to go out of bounds. simulate fake collision with world boundary
            const Vector2i closestPointInBounds = System::getGame().getScene().getClosestPositionInBounds(newPositionInt);
            trans.setPosition(closestPointInBounds.as<f32>(), mSelf);
            ColliderSystem::updatePosition(mSelf, getShapeMutable(), trans, getOffset());
        }

    } else {
        trans.setPosition(newPosition, mSelf);
    }

    if (mSelf.has<Trigger>()) {
        auto trigger = mSelf.get<Trigger>();
        trigger.shape.setPosition(trans, trigger.offset);
        mSelf.set(trigger);
    }
}

bool Collider::emitCollisionInfo(const Vector2f amount, const HitInfo hitinfo, bool isX, bool updateRigidBodyFlags) {
    bool skipBounceStep = false;
    Velocity& velocity = mSelf.get<Velocity>();

    if (updateRigidBodyFlags && !isX) {
        auto& rigidbody = mSelf.get<RigidBody>();
        const bool wasGrounded = rigidbody.isGrounded;
        auto jumpControlOpt = mSelf.tryGet<Jumper>();
        auto momentumOpt = mSelf.tryGet<Momentum>();
        const bool hasMomentum = momentumOpt && momentumOpt->isMomentumStored();

        if (hitinfo && hitinfo.isVertical()) {
            // update states for grounded, jumping, and reset impulses
            if (amount.y <= 0 && hitinfo.isDown()) {
                rigidbody.setGrounded(hitinfo.otherMaterial);
            } else {
                rigidbody.setNotGrounded();
            }

            if (jumpControlOpt) {
                jumpControlOpt->isJumping = false;
            }
            velocity.residualImpulse.y = 0;
        } else if (!hitinfo) {
            rigidbody.setNotGrounded();
        }

        // UPDATE RIGIDBODY FLAGS, MOMENTUM, AND COYOTE TIME
        if (rigidbody.isGrounded) {
            rigidbody.isLanding = !wasGrounded;
            if (rigidbody.isLanding) {
                Event.emit<evt::Landing>(mSelf);
                rigidbody.framesSinceLanding = 0;
            }

            if (hasMomentum) {
                momentumOpt->onMomentumNotUsed();
            }

            if (velocity.total.y < 0 && wasGrounded && (!jumpControlOpt || !jumpControlOpt->isJumping)) {
                // zero y velocity when grounded and not trying to jump, otherwise entity falls at terminal velocity after walking off platform
                // do this on second frame on the ground
                velocity.stable.y = 0;
                skipBounceStep = true;
            }

        } else {
            if (jumpControlOpt) {
                if (wasGrounded && !jumpControlOpt->isJumping) {
                    jumpControlOpt->coyoteSecondsRemaining = jumpControlOpt->coyoteTimeSecondsMax;
                } else if (jumpControlOpt->coyoteSecondsRemaining > 0) {
                    // is jumping
                    jumpControlOpt->coyoteSecondsRemaining -= Time.dt();
                    skipBounceStep = true;
                }
            }

            // prevent repeated push forces from accumulating huge speed
            if (hasMomentum && momentumOpt->cooldownFrames <= 0) {
                velocity.stable += momentumOpt->getMomentum();
                momentumOpt->resetMomentum();
                momentumOpt->cooldownFrames = MOMENTUM_COOLDOWN_FRAMES;
            } else if (momentumOpt && momentumOpt->cooldownFrames > 0) {
                momentumOpt->cooldownFrames--;
            }
        }

        if (jumpControlOpt) {
            // mSelf.set(*jumpControlOpt);
            // this is 2x faster:
            mSelf.get<Jumper>() = *jumpControlOpt;
        }
        if (momentumOpt) {
            // mSelf.set(*momentumOpt);
            // this is 2x faster:
            mSelf.get<Momentum>() = *momentumOpt;
        }
    }

    if (hitinfo) {
        // BOUNCING
        // RESEARCH - using relative velocity instead of the mover's velocity would also be more accurate

        // average bounciness of both colliders
        const f32 selfBounciness = MaterialData::get(mMaterial).bounciness;
        const f32 otherBounciness = MaterialData::get(hitinfo.otherMaterial).bounciness;
        const f32 bounciness = mSelf.has<IsIdealSpring>() ? 1.0f : (selfBounciness + otherBounciness) / 2.0f;
        if (!skipBounceStep && bounciness != 0.0 && mSelf.has<Velocity>()) {
            if ((isX && math::abs(velocity.total.x) >= BOUNCE_THRESHOLD) || (!isX && math::abs(velocity.total.y) >= BOUNCE_THRESHOLD)) {
                // stable can be negative (like for gravity) when impulse makes total velocity positive.
                // in that case we don't want to do anything
                if (isX) {
                    if (math::sign(velocity.stable.x) == math::sign(velocity.total.x)) {
                        velocity.stable.x = velocity.stable.x * -bounciness;
                    }

                } else {
                    if (math::sign(velocity.stable.y) == math::sign(velocity.total.y)) {
                        velocity.stable.y = velocity.stable.y * -bounciness;
                    }
                }

                // do a post check in case an external force like gravity makes the first check always pass
                if (isX) {
                    if (math::abs(velocity.stable.x) < BOUNCE_THRESHOLD) {
                        velocity.stable.x = 0;
                    }

                } else {
                    if (math::abs(velocity.stable.y) < BOUNCE_THRESHOLD) {
                        velocity.stable.y = 0;
                    }
                }
            }
        } else {
            // zero velocity in collision direction
            constexpr f32 maxSpeedPostCollision = 20.0f;
            if (hitinfo.isVertical() && !isX) {
                if (math::abs(velocity.stable.y) > maxSpeedPostCollision) {
                    velocity.stable.y = maxSpeedPostCollision * math::sign(velocity.stable.y);
                }
            } else if (hitinfo.isHorizontal() && isX) {
                if (math::abs(velocity.stable.x) > maxSpeedPostCollision) {
                    velocity.stable.x = maxSpeedPostCollision * math::sign(velocity.stable.x);
                }
            }
        }
        return true;
    }
    return false;
}

// dispatches correct move method based on Collision Layer
/* args
    - const Vector2f amount: float amount to move, in pixels
    - const CollisionCallbackNew callback: callback to run on *this* if a collision occurs
    - bool isGroundedCheckNeeded: skips grounded check if false
    - bool isManualMove: should be true if this is called outside of the physics system. If true, entity transform is updated here.
    - bool isSkipMomentumUpdate: if true, pushing other colliders will not affect their momentum value (but will reset its timer)
*/
bool Collider::move(const Vector2f amount, const CollisionCallback callback, bool isGroundedCheckNeeded, bool isManualMove, bool isPushedBySolid,
                    bool updateRigidBodyFlags, bool isSkipmomentumUpdate) {
    // round to nearest pixel
    mXRemainder += amount.x;
    mYRemainder += amount.y;

    Vector2i toMoveRounded = Vector2i(std::round(mXRemainder), std::round(mYRemainder));
    // only return early if we don't need a grounded check (solids can never be grounded)
    if (toMoveRounded.x == 0 && toMoveRounded.y == 0 && (!isGroundedCheckNeeded || isSolid())) {
        return false;
    }
    mXRemainder -= toMoveRounded.x;
    mYRemainder -= toMoveRounded.y;

    bool isHit = false;
    switch (mCollisionLayer) {
    case CollisionLayer::Solid: {
        // check riding status *before* moving
        const auto riding = getRidingCollidersQT();

        // nothing can stop solids, so do full movement immediately and emit nothing
        // need to move+push on one axis before moving on the other
        auto moveVec = Vector2i(toMoveRounded.x, 0);
        moveNoCollisionCheck(amount, moveVec);
        pushAndCarry1D(amount, moveVec, riding, isSkipmomentumUpdate);

        moveVec = Vector2i(0, toMoveRounded.y);
        moveNoCollisionCheck(amount, moveVec);
        pushAndCarry1D(amount, moveVec, riding, isSkipmomentumUpdate);
        break;
    }
    case CollisionLayer::SemiSolid: {
        // check riding status *before* moving
        const auto riding = getRidingCollidersQT();

        // moveX, then push/carry in that direction only
        const auto collidersInArea = getCollidersInMoveArea(toMoveRounded, getCollisionLayersThatCanStopMe(), updateRigidBodyFlags);
        auto originalPosition = getShape().getPosition();
        isHit = emitCollisionInfo(amount, moveX(amount, toMoveRounded, callback, collidersInArea), true, false);
        Vector2i moveAmount = getShape().getPosition() - originalPosition;
        Vector2f moveUnrounded = (isHit || mSelf.has<Wiggle>()) ? moveAmount.as<f32>() : Vector2f(amount.x, 0);
        pushAndCarry1D(moveUnrounded, moveAmount, riding, isSkipmomentumUpdate, isPushedBySolid);

        // moveY, then push/carry in that direction only
        originalPosition = getShape().getPosition();
        const bool isHitY =
            emitCollisionInfo(amount, moveY(amount, toMoveRounded, callback, collidersInArea, isGroundedCheckNeeded), false, updateRigidBodyFlags);
        isHit = isHit || isHitY;
        moveAmount = getShape().getPosition() - originalPosition;
        moveUnrounded = (isHitY || mSelf.has<Wiggle>()) ? moveAmount.as<f32>() : Vector2f(0, amount.y);
        pushAndCarry1D(moveUnrounded, moveAmount, riding, isSkipmomentumUpdate, isPushedBySolid);

        break;
    }
    default: {
        const auto collidersInArea = getCollidersInMoveArea(toMoveRounded, getCollisionLayersThatCanStopMe(), updateRigidBodyFlags);
        isHit = emitCollisionInfo(amount, moveX(amount, toMoveRounded, callback, collidersInArea), true, false);
        isHit =
            emitCollisionInfo(amount, moveY(amount, toMoveRounded, callback, collidersInArea, isGroundedCheckNeeded), false, updateRigidBodyFlags) ||
            isHit;
        break;
    }
    }

    if (isManualMove) {
        // if we moved outside of the physics system we can update other components immediately
        updateEntityPosition();
    }

    return isHit;
}

HitInfo Collider::moveX(const Vector2f amount, const Vector2i amountRounded, const CollisionCallback callback,
                        const std::vector<std::pair<ecs::Entity, Collider>>& others) {
    s32 toMove = amountRounded.x;

    if (toMove == 0) {
        return HitInfo();
    }

    const AABB originalShape = mShape;
    const s32 moveSign = math::sign(toMove);
    const auto moveNormal = Vector2i(moveSign, 0);
    while (toMove != 0) {
        auto nextPos = mShape.getPosition() + moveNormal;
        auto hitInfo = checkCollisionInMoveArea(nextPos, moveNormal, others, true);

        if (!hitInfo) {
            mShape.setPosition(nextPos);
            toMove -= moveSign;
        } else {
            // Try to wiggle out of it
            if (auto wiggleOpt = mSelf.tryGet<Wiggle>(); wiggleOpt) {
                if (wiggleOpt->callback(*wiggleOpt, *this, hitInfo, moveNormal, amount)) {
                    continue;
                }
            }

            // Collision happened
            ColliderSystem::updateShape(mSelf, originalShape, mShape);
            if (callback != nullptr) {
                callback(getEntity(), hitInfo.getOther(), moveNormal);
            }
            mXRemainder = 0;
            return hitInfo;
        }
    }
    ColliderSystem::updateShape(mSelf, originalShape, mShape);
    return HitInfo();
}

HitInfo Collider::moveY(const Vector2f amount, const Vector2i amountRounded, const CollisionCallback callback,
                        const std::vector<std::pair<ecs::Entity, Collider>>& others, bool isGroundedCheckNeeded) {
    // include fractional movement from previous calls
    s32 toMove = amountRounded.y;
    std::vector<std::pair<ecs::Entity, Collider>> groundColliders;
    if (isGroundedCheckNeeded) {
        for (const auto& pair : others) {
            if (isOtherGround(pair.second)) {
                groundColliders.push_back(pair);
            }
        }
    }

    auto groundedCheck = [this](f32 amountY, const std::vector<std::pair<ecs::Entity, Collider>>& groundColliders) -> HitInfo {
        if (amountY > 0) {
            return HitInfo();
        }

        return checkIsGrounded(true, groundColliders);
    };

    if (toMove == 0) {
        if (isGroundedCheckNeeded) {
            return groundedCheck(amount.y, groundColliders);
        }
        return HitInfo();
    }

    const AABB originalShape = mShape;
    const s32 moveSign = math::sign(toMove);
    const auto moveNormal = Vector2i(0, moveSign);
    while (toMove != 0) {
        auto nextPos = mShape.getPosition() + moveNormal;
        auto hitInfo = checkCollisionInMoveArea(nextPos, moveNormal, others, true);

        if (!hitInfo) {
            mShape.setPosition(nextPos);
            toMove -= moveSign;
        } else {
            // try to wiggle out of it
            if (auto wiggleOpt = mSelf.tryGet<Wiggle>(); wiggleOpt) {
                if (wiggleOpt->callback(*wiggleOpt, *this, hitInfo, moveNormal, amount)) {
                    continue;
                }
            }

            // collision happened
            ColliderSystem::updateShape(mSelf, originalShape, mShape);
            if (callback != nullptr) {
                callback(getEntity(), hitInfo.getOther(), moveNormal);
            }
            mYRemainder = 0;
            return hitInfo;
        }
    }

    ColliderSystem::updateShape(mSelf, originalShape, mShape);

    if (isGroundedCheckNeeded) {
        return groundedCheck(amount.y, groundColliders);
    }
    return HitInfo();
}

void Collider::moveNoCollisionCheck(Vector2f toMove, Vector2i toMoveRounded) {
    const AABB previousShape = mShape;
    mShape.setPosition(mShape.getPosition() + toMoveRounded);
    ColliderSystem::updateShape(mSelf, previousShape, mShape);
}

void Collider::pushAndCarry1D(Vector2f moveOriginal, Vector2i move1D, const std::vector<Collider*>& ridingColliders, bool isManualMove,
                              bool isPushedBySolid, bool isSkipMomentumUpdate) {
    if (!mIsCollidable) {
        return;
    }
    // turn off collision so colliders moved by us don't get stuck on us
    mIsCollidable = false;

    // Caller should only have moved on one dimension before calling this, so only push/carry on that dimension
    if (move1D.x > 0) {
        _pushAndCarry(move1D.x, moveOriginal.x, true, mShape.right(), &AABB::left, ridingColliders, isManualMove, isPushedBySolid,
                      isSkipMomentumUpdate);
    } else if (move1D.x < 0) {
        _pushAndCarry(move1D.x, moveOriginal.x, true, mShape.left(), &AABB::right, ridingColliders, isManualMove, isPushedBySolid,
                      isSkipMomentumUpdate);
    } else if (move1D.y > 0) {
        _pushAndCarry(move1D.y, moveOriginal.y, false, mShape.top(), &AABB::bottom, ridingColliders, isManualMove, isPushedBySolid,
                      isSkipMomentumUpdate);
    } else if (move1D.y < 0) {
        _pushAndCarry(move1D.y, moveOriginal.y, false, mShape.bottom(), &AABB::top, ridingColliders, isManualMove, isPushedBySolid,
                      isSkipMomentumUpdate);
    }

    mIsCollidable = true;
}

// we are moving, other is still.
bool Collider::isCollisionPossible(const Collider& other, const Vector2i moveNormal, const u16 layerMask) const {
    return other.mIsCollidable && this != &other && LAYER_MATRIX.isOn(mCollisionLayer, other.mCollisionLayer) &&
           (layerMask & other.mCollisionLayer) > 0 && checkDirectionalCollision(mShape, other.mShape, moveNormal, other.getCollisionDir());
}

// other is moving, we are still. Only affects directional collision check.
bool Collider::isCollisionPossibleReversed(const Collider* other, const Vector2i moveNormal, const u16 layerMask) const {
    return other->mIsCollidable && this != other && LAYER_MATRIX.isOn(mCollisionLayer, other->mCollisionLayer) &&
           (layerMask & other->mCollisionLayer) > 0 && checkDirectionalCollision(other->mShape, mShape, moveNormal, getCollisionDir());
}

// Check for collision 1 unit down.
HitInfo Collider::checkIsGrounded(const bool triggerCollisionEvents, const std::vector<std::pair<ecs::Entity, Collider>>& groundColliders) {
    const Vector2i nextPos = mShape.getPosition() + Vector2i::DOWN;
    return checkCollisionInMoveArea(nextPos, Vector2i::DOWN, groundColliders, triggerCollisionEvents);
}

// check other's collision layer and direction.
bool Collider::isOtherGround(const Collider& other) const {
    // semisolids should be ground for each other, so just check if other is any solid? Works ig.

    // return (other->mCollisionLayer & getCollisionLayersThatCanStopMe()) > 0 &&
    return (other.isSolidAny() && (other.mCollisionDir == CollisionDir::ALL || other.mCollisionDir == CollisionDir::UP));
}

// get colliders that are riding us. Basically check which colliders would intersect us if we moved 1px up, taking collision layers and directional
// collision into account.
std::vector<Collider*> Collider::getRidingCollidersQT() const {
    std::vector<Collider*> riding;
    const auto movedCollider = AABB(mShape.getPosition() + Vector2i::UP, mShape.getHalf());
    const auto layerMask = getCollisionLayersThatCanRideMe();

    for (auto entity : ColliderSystem::query(movedCollider)) {
        const auto pCollider = &entity.get<Collider>();

        // (making sure to not use the moved collider for the directional collision check so the edges are properly aligned)
        if (isCollisionPossibleReversed(pCollider, {0, -1}, layerMask)) {
            riding.push_back(pCollider);
        }
    }
    return riding;
}

u16 Collider::getCollisionLayersThatCanStopMe() const {
    // Special case: actors shouldn't stop SemiSolid colliders from moving.
    // SemiSolids should also push each other instead of stopping movement.
    // Collision layer matrix is also checked in checkCollision. This is an additional check that must pass for a collision to happen, so returning
    // ALL as default is fine. Only add something here for special stuff like asymmetric behavior.
    if (mCollisionLayer == CollisionLayer::SemiSolid) {
        return CollisionLayer::Solid;
    } else if (mCollisionLayer == CollisionLayer::Solid) {
        return CollisionLayer::None;
    } else {
        return CollisionLayer::ALL;
    }
}

u16 Collider::getCollisionLayersThatCanRideMe() const {
    if (isSolidAny()) {
        return CollisionLayer::SemiSolid | CollisionLayer::Actor;
    } else {
        return CollisionLayer::None;
    }
}

void Collider::setOffset(Vector2i offset) {
    mOffset = offset;

    // need to recalculate shape's position with this offset
    // it's ok if entity doesn't have a transform yet, that means it isn't in the ColliderSystem yet,
    // and this calculation will happen when its `onAdd` method runs this->setEntity()
    if (mSelf.has<Transform>()) {
        mShape = AABB(mSelf.get<Transform>(), mShape.getHalf(), mOffset);
    }
}

void Collider::_pushAndCarry(s32 toMoveRounded, f32 toMoveUnrounded, bool isXDirection, s32 solidEdge, EdgeGetter edgeFunc,
                             const std::vector<Collider*>& riding, bool isManualMove, bool isPushedBySolid, bool isSkipMomentumUpdate) {
    Vector2i moveVec;
    if (isXDirection) {
        moveVec = {toMoveRounded, 0};
    } else {
        moveVec = {0, toMoveRounded};
    }
    const auto prevColliderState = Collider(Transform::world(mShape.getPosition() - moveVec), mShape.getHalf(), mCollisionLayer,
                                            ColliderParams{
                                                .material = mMaterial,
                                                .collisionDir = mCollisionDir,
                                                .onCollisionEnter = nullptr,
                                            });

    assert(math::abs(static_cast<f32>(toMoveRounded) - toMoveUnrounded) <= 1 && "Rounding anomaly");

    std::vector<Collider*> toCarry = riding;
    const u16 notSolidMask = ~CollisionLayer::Solid;  // cannot be pushed or carried
    for (auto entity : ColliderSystem::query(mShape)) {
        Collider* other = &entity.get<Collider>();
        if (this != other && prevColliderState.isCollisionPossibleReversed(other, moveVec * -1, notSolidMask)) {
            // push takes priority over carry
            auto it = ecs::whal_find(toCarry.begin(), toCarry.end(), other);
            if (it != toCarry.end()) {
                toCarry.erase(it);
            }

            s32 actorEdge = (other->getShape().*edgeFunc)();
            s32 toMoveOverlap = solidEdge - actorEdge;
            Vector2i otherMoveVec = isXDirection ? Vector2i(toMoveOverlap, 0) : Vector2i(0, toMoveOverlap);

            // If we are a semisolid pushing another semisolid, and a solid is not pushing us, then `other` may "push back" on us.
            // If a solid is pushing us though, then `other` is effectively being pushed by a solid.
            if (!isPushedBySolid && isSemiSolid()) {
                Vector2i originalPosition = other->getShape().getPosition();
                bool hitSolid = false;
                hitSolid =
                    other->move(otherMoveVec.as<f32>(), &squishEntityPushedBySemiSolid, false, isManualMove, false, false, isSkipMomentumUpdate);

                Vector2i newPosition = other->getShape().getPosition();
                // Calculate difference between newPosition and expected position.
                // If we didn't hit something, but delta is nonzero, then something that `other` pushed hit a solid.
                auto delta = ((originalPosition + otherMoveVec) - newPosition) * -1;
                if (other->mIsAlive && other->isSemiSolid() && (hitSolid || delta.x != 0 || delta.y != 0)) {
                    // if other didn't move the full amount, it must have hit a solid, so push *this* back by the difference
                    // using &squishCollider as the callback because we're effectively being pushed by the solid that `other` hit
                    mIsCollidable = true;
                    other->mIsCollidable = false;
                    move(delta.as<f32>(), &squishEntity, false, isManualMove, true, false, isSkipMomentumUpdate);
                    mIsCollidable = false;
                    other->mIsCollidable = true;

                    if (!mIsAlive) {
                        break;
                    }

                    // update move vars so the rest of the colliders get pushed by the new amount
                    // for the colliders that were already pushed/carried, it is what it is :)
                    // update move amounts so we don't push the next colliders by too much
                    moveVec += delta;
                    if (isXDirection) {
                        solidEdge = toMoveRounded > 0 ? mShape.right() : mShape.left();
                        toMoveRounded = moveVec.x;
                        toMoveUnrounded += delta.x;
                    } else {
                        solidEdge = toMoveRounded > 0 ? mShape.top() : mShape.bottom();
                        toMoveRounded = moveVec.y;
                        toMoveUnrounded += delta.y;
                    }
                }
            } else {
                other->move(otherMoveVec.as<f32>(), &squishEntity, false, isManualMove, true, false, isSkipMomentumUpdate);
            }

            // emit push event
            HitInfo hitinfo(moveVec, false, true);
            hitinfo.setOther(other->getEntity());
            hitinfo.otherMaterial = other->getMaterial();
            hitinfo.otherLayer = other->getCollisionLayer();
            Event.emit<evt::Collision>(mSelf, hitinfo);

            if (other->mSelf.has<Momentum>()) {
                // set momentum if this movement was part of the physics system
                if (isSkipMomentumUpdate) {
                    if (isXDirection) {
                        other->mSelf.get<Momentum>().maintainMomentumX();
                    } else {
                        other->mSelf.get<Momentum>().maintainMomentumY();
                    }

                } else {
                    // don't worry about rounding, we're just moving the overlap distance
                    f32 momentum = static_cast<f32>(toMoveRounded) / Time.dt();
                    if (isXDirection) {
                        other->getEntity().get<Momentum>().setMomentumX(other->getEntity(), momentum);
                    } else {
                        other->getEntity().get<Momentum>().setMomentumY(other->getEntity(), momentum);
                    }
                }
            }
        }
    }

    // carry all riders that weren't pushed
    for (auto other : toCarry) {
        // I might change this for solids moving down faster than gravity RESEARCH
        if (isXDirection) {
            other->move(Vector2f(toMoveRounded, 0), nullptr);
        } else {
            other->move(Vector2f(0, toMoveRounded), nullptr);
        }

        // emit carry event
        HitInfo hitinfo({0, 1}, false, false, true);
        hitinfo.setOther(other->getEntity());
        hitinfo.otherMaterial = other->getMaterial();
        hitinfo.otherLayer = other->getCollisionLayer();
        Event.emit<evt::Collision>(mSelf, hitinfo);

        if (other->getEntity().has<Momentum>()) {
            // set momentum if this movement was part of the physics system
            if (isManualMove) {
                if (isXDirection) {
                    other->getEntity().get<Momentum>().maintainMomentumX();

                } else {
                    other->getEntity().get<Momentum>().maintainMomentumY();
                }
                // other->maintainMomentum(isXDirection);
            } else {
                f32 momentum = toMoveUnrounded / Time.dt();
                if (isXDirection) {
                    other->getEntity().get<Momentum>().setMomentumX(other->getEntity(), momentum);
                } else {
                    other->getEntity().get<Momentum>().setMomentumY(other->getEntity(), momentum);
                }
            }
        }
    }
}

// collision layers should already have been checked by caller
HitInfo Collider::checkCollisionInMoveArea(const Vector2i newPosition, const Vector2i moveNormal,
                                           const std::vector<std::pair<ecs::Entity, Collider>>& others, const bool triggerCollisionEvents) const {
    const auto movedCollider = AABB(newPosition, mShape.getHalf());
    HitInfo hitInfoToReturn;  // used for updating rigidbody flags n such. doesn't matter which specific collision is returned.

    for (const auto& pair : others) {
        const auto entity = pair.first;
        const auto& otherCollider = pair.second;
        if (!checkDirectionalCollision(mShape, otherCollider.mShape, moveNormal, otherCollider.getCollisionDir())) {
            continue;
        }

        HitInfo hitInfo = movedCollider.collide(otherCollider.getShape());
        if (hitInfo) {
            hitInfo.setOther(entity);
            hitInfo.otherLayer = otherCollider.getCollisionLayer();
            hitInfo.otherMaterial = otherCollider.getMaterial();

            // only care about the hit flag for the direction we're moving in (in the case of a corner hit)
            if (moveNormal.x != 0) {
                hitInfo.clearVerticalFlags();
            } else {
                hitInfo.clearHorizontalFlags();
            }

            hitInfoToReturn = hitInfo;
            if (triggerCollisionEvents) {
                Event.emit<evt::Collision>(mSelf, hitInfo);
            }
        }
    }
    return hitInfoToReturn;
}

HitInfo Collider::checkCollisionQT(const Vector2i position, const Vector2i moveNormal, const u16 layerMask, const bool triggerCollisionEvents) const {
    if (!isCollidable()) {
        return HitInfo();
    }
    const auto movedCollider = AABB(position, mShape.getHalf());
    HitInfo hitInfoToReturn;  // used for updating rigidbody flags n such. doesn't matter which specific collision is returned.

    for (auto entity : ColliderSystem::query(movedCollider)) {
        const auto& collider = entity.get<Collider>();
        if (!isCollisionPossible(collider, moveNormal, layerMask)) {
            continue;
        }

        HitInfo hitInfo = movedCollider.collide(collider.getShape());
        if (hitInfo) {
            hitInfo.setOther(entity);
            hitInfo.otherLayer = collider.getCollisionLayer();
            hitInfo.otherMaterial = collider.getMaterial();

            // only care about the hit flag for the direction we're moving in (in the case of a corner hit)
            if (moveNormal.x != 0) {
                hitInfo.clearVerticalFlags();
            } else {
                hitInfo.clearHorizontalFlags();
            }

            hitInfoToReturn = hitInfo;
            if (triggerCollisionEvents) {
                Event.emit<evt::Collision>(mSelf, hitInfo);
            }
        }
    }

    return hitInfoToReturn;
}

std::vector<std::pair<ecs::Entity, Collider>> Collider::getCollidersInMoveArea(const Vector2i toMove, const u16 layerMask,
                                                                               bool updateRigidBodyFlags) const {
    if (!isCollidable()) {
        return {};
    }

    s32 yPadding = 0;
    if (updateRigidBodyFlags) {
        // add 1 to toMove.y so ground colliders are fetched
        yPadding = 1;
    }
    const auto bigCollider =
        AABB(mShape.getPosition() + toMove / 2, mShape.getHalf() + Vector2i(std::ceil(static_cast<f32>(math::abs(toMove.x)) / 2),
                                                                            std::ceil(static_cast<f32>(math::abs(toMove.y)) / 2) + yPadding));

    std::vector<std::pair<ecs::Entity, Collider>> toReturn;
    for (auto entity : ColliderSystem::query(bigCollider)) {
        const auto& other = entity.get<Collider>();
        // quick and dirty check for collision layers; ignoring directional collision
        bool isCollidable = other.mIsCollidable && this != &other && LAYER_MATRIX.isOn(mCollisionLayer, other.mCollisionLayer) &&
                            (layerMask & other.mCollisionLayer) > 0;
        if (!isCollidable) {
            continue;
        }
        toReturn.push_back({entity, other});
    }

    return toReturn;
}

void Collider::squish(ecs::Entity other, Vector2i hitNormal) {
    mSquishCallback(mSelf, other, hitNormal);
}

// Try to wiggle out of collision if barely clipping another collider.
// Returns true if successful.
// Could be a lot faster if I do a broad pass QuadTree check like i do for normal movement
bool Collider::tryCornerCorrection(Vector2i nextPosition, s32 moveSign, Vector2i moveNormal, Vector2i correctionBuffer) {
    assert(correctionBuffer.x >= 0 && correctionBuffer.y >= 0 && "Corner correction buffer should only have positive integers");
    if (moveNormal.y != 0) {
        for (s32 i = 1; i <= correctionBuffer.x; i += 1) {
            for (s32 mult : {-1, 1}) {
                Vector2i nextPos = nextPosition + Vector2i(i * mult, 0);
                if (!checkCollisionQT(nextPos, moveNormal)) {
                    mShape.setPosition(nextPos);
                    return true;
                }
            }
        }
    } else if (moveNormal.x != 0) {
        for (s32 i = 1; i <= correctionBuffer.y; i += 1) {
            for (s32 mult : {-1, 1}) {
                Vector2i nextPos = nextPosition + Vector2i(0, i * mult);
                if (!checkCollisionQT(nextPos, moveNormal)) {
                    mShape.setPosition(nextPos);
                    return true;
                }
            }
        }
    }
    return false;
}

void Collider::loadImpl(ecs::Entity entity, void* data) {
    const LoadContext& ctx = *static_cast<LoadContext*>(data);

    Collider collider = entity.has<Collider>() ? entity.get<Collider>() : Collider{};
    CollisionDir collisionDir = collider.getCollisionDir();
    WorldMaterial material = collider.getMaterial();

    if (tryReadVal(ctx.values, "CollisionDir", &collisionDir)) {
        collider.setCollisionDir(collisionDir);
    }
    if (tryReadVal(ctx.values, "Material", &material)) {
        collider.setMaterial(material);
    }

    std::string layerName;
    if (tryReadVal(ctx.values, "Layer", &layerName)) {
        collider.setCollisionLayer(CollisionLayer::fromString(layerName.c_str()));
    }

    collider.setShape(readShapeOrDefault(ctx, entity, "Shape", &collider.mOffset).getAABB());
    entity.add(collider);
}

void Momentum::setMomentumX(ecs::Entity self, const f32 momentumX) {
    auto eRB = self.tryGet<RigidBody>();
    if (!eRB) {
        return;
    }

    storedMomentum[nextIx.x].x = momentumX * eRB->momentumMultiplier.x;
    nextIx.x++;
    if (nextIx.x == MOMENTUM_STORAGE_COUNT) {
        nextIx.x = 0;
    }
    momentumFramesLeft.x = MOMENTUM_LIFETIME_FRAMES;
}

void Momentum::setMomentumY(ecs::Entity self, const f32 momentumY) {
    auto eRB = self.tryGet<RigidBody>();
    if (!eRB) {
        return;
    }

    storedMomentum[nextIx.y].y = momentumY * eRB->momentumMultiplier.y;
    nextIx.y++;
    if (nextIx.y == MOMENTUM_STORAGE_COUNT) {
        nextIx.y = 0;
    }
    momentumFramesLeft.y = MOMENTUM_LIFETIME_FRAMES;
}

void Momentum::maintainMomentumX() {
    momentumFramesLeft.x = MOMENTUM_LIFETIME_FRAMES;
}

void Momentum::maintainMomentumY() {
    momentumFramesLeft.y = MOMENTUM_LIFETIME_FRAMES;
}

void Momentum::onMomentumNotUsed() {
    momentumFramesLeft.x -= 1;
    if (momentumFramesLeft.x == 0) {
        resetMomentumX();
    }

    momentumFramesLeft.y -= 1;
    if (momentumFramesLeft.y == 0) {
        resetMomentumY();
    }
}

void Momentum::resetMomentumX() {
    momentumFramesLeft.x = 0;
    for (size_t i = 0; i < MOMENTUM_STORAGE_COUNT; i++) {
        storedMomentum[i].x = 0.0f;
    }
    nextIx.x = 0;
}

void Momentum::resetMomentumY() {
    momentumFramesLeft.y = 0;
    for (size_t i = 0; i < MOMENTUM_STORAGE_COUNT; i++) {
        storedMomentum[i].y = 0.0f;
    }
    nextIx.y = 0;
}

Vector2f Momentum::getMomentum() const {
    Vector2f momentum;

    if (isMomentumStoredX()) {
        f32 sum = 0;
        f32 count = 0;
        for (size_t i = 0; i < MOMENTUM_STORAGE_COUNT; i++) {
            sum += storedMomentum[i].x;
            count += 1.0f;
        }
        if (count > 0.0f) {
            momentum.x = sum / count;
        }
    }

    if (isMomentumStoredY()) {
        f32 sum = 0;
        f32 count = 0;
        for (size_t i = 0; i < MOMENTUM_STORAGE_COUNT; i++) {
            sum += storedMomentum[i].y;
            count += 1.0f;
        }
        if (count > 0.0f) {
            momentum.y = sum / count;
        }
    }

    return momentum;
}

}  // namespace whal

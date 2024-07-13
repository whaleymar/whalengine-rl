#include "Collision.h"

#include <cmath>

#include "ECS/PlayerControl.h"
#include "ECS/RigidBody.h"
#include "ECS/Systems/CollisionManager.h"
#include "ECS/Tags.h"
#include "ECS/Transform.h"
#include "ECS/TriggerZone.h"
#include "ECS/Velocity.h"

#include "Events/Events.h"
#include "Game.h"
#include "Physics/CollisionLayer.h"
#include "Physics/CollisionUtil.h"
#include "Physics/Material.h"
#include "Settings.h"

#include "Physics/HitInfo.h"
#include "Systems/System.h"
#include "Util/MathUtil.h"
#include "Util/Print.h"

namespace whal {

constexpr s32 MOMENTUM_LIFETIME_FRAMES = 10;
constexpr s32 MOMENTUM_COOLDOWN_FRAMES = 10;
constexpr s32 CORNERCORRECTIONWIGGLE = 3 * PIXELS_PER_TEXEL;
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
bool defaultWiggle(Collider* callbackCollider, HitInfo hitinfo, Vector2i moveNormal, Vector2f fullMoveAmount) {
    const s32 moveSign = moveNormal.x() != 0 ? sign(moveNormal.x()) : sign(moveNormal.y());
    auto nextPos = callbackCollider->getShape().getPosition() + moveNormal;
    if (hitinfo.isUp() && moveSign == 1 && (hitinfo.otherLayer & (callbackCollider->getCollisionLayersThatCanStopMe())) > 0) {
        return callbackCollider->tryCornerCorrection(nextPos, fullMoveAmount.x(), moveNormal);
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

Collider::Collider(AABB shape, CollisionLayer::Layer layer, WorldMaterial material, CollisionCallback onCollisionEnter_, CollisionDir collisionDir,
                   CollisionCallback squish_)
    : mShape(shape), mCollisionLayer(layer), mOnCollisionEnter(onCollisionEnter_), mSquishCallback(squish_), mMaterial(material),
      mCollisionDir(collisionDir) {}

Collider::Collider(Transform2D transform, Vector2i halflen, CollisionLayer::Layer layer, WorldMaterial material, CollisionCallback onCollisionEnter_,
                   CollisionDir collisionDir, CollisionCallback squish_)
    : mShape(AABB(transform, halflen)), mCollisionLayer(layer), mOnCollisionEnter(onCollisionEnter_), mSquishCallback(squish_), mMaterial(material),
      mCollisionDir(collisionDir) {}

Collider Collider::Actor(AABB shape, CollisionCallback squish_) {
    auto collider = Collider(shape, CollisionLayer::Actor);
    collider.setSquishCallback(squish_);
    return collider;
}

Collider Collider::Actor(Transform2D transform, Vector2i halflen, CollisionCallback squish_) {
    auto collider = Collider(transform, halflen, CollisionLayer::Actor);
    collider.setSquishCallback(squish_);
    return collider;
}

Collider Collider::Solid(AABB shape, WorldMaterial material, CollisionCallback onCollisionEnter_, CollisionDir collisionDir,
                         CollisionCallback squish_) {
    return Collider(shape, CollisionLayer::Solid, material, onCollisionEnter_, collisionDir, squish_);
}

Collider Collider::Solid(Transform2D transform, Vector2i halflen, WorldMaterial material, CollisionCallback onCollisionEnter_,
                         CollisionDir collisionDir, CollisionCallback squish_) {
    return Collider(transform, halflen, CollisionLayer::Solid, material, onCollisionEnter_, collisionDir, squish_);
}

Collider Collider::SemiSolid(AABB shape, WorldMaterial material, CollisionCallback onCollisionEnter_, CollisionDir collisionDir,
                             CollisionCallback squish_) {
    return Collider(shape, CollisionLayer::SemiSolid, material, onCollisionEnter_, collisionDir, squish_);
}

Collider Collider::SemiSolid(Transform2D transform, Vector2i halflen, WorldMaterial material, CollisionCallback onCollisionEnter_,
                             CollisionDir collisionDir, CollisionCallback squish_) {
    return Collider(transform, halflen, CollisionLayer::SemiSolid, material, onCollisionEnter_, collisionDir, squish_);
}

void Collider::setCollisionCallback(CollisionCallback callback) {
    mOnCollisionEnter = callback;
}

// syncs other engine components (Transform2D, PrecisePosition, and Trigger) with collider position
void Collider::updateEntityPosition() {
    Transform2D& trans = mSelf.get<Transform2D>();
    auto const shape = getShape();
    auto const newPosition = centerToTrans(shape.getPosition(), shape.getHalf(), trans.rotationDegrees);

    // make sure player(s) can't go out of bounds
    if (mSelf.has<Player>()) {
        if (Game::instance().getScene().getLevelAt(newPosition)) {
            trans.position = newPosition;
        } else {
            // tried to go out of bounds. simulate fake collision with world boundary
            auto closestPointInBounds = Game::instance().getScene().getClosestPositionInBounds(newPosition);
            trans.position = closestPointInBounds;
            QuadTreeSystem::updatePosition(mSelf, &getShapeMutable(), trans);
        }

    } else {
        trans.position = newPosition;
    }

    if (auto precisePositionOpt = mSelf.tryGet<PrecisePosition>(); precisePositionOpt) {
        (*precisePositionOpt)->position = toFloatVec(trans.position);
    }

    if (mSelf.has<Trigger>()) {
        auto trigger = mSelf.get<Trigger>();
        Transform2D adjustedTransform = Transform2D(trans.position + trigger.offset);
        trigger.shape.setPosition(adjustedTransform);
        mSelf.set(trigger);
    }
}

bool Collider::emitCollisionInfo(const Vector2f amount, const HitInfo hitinfo, bool isX, bool updateRigidBodyFlags) {
    bool skipBounceStep = false;

    if (updateRigidBodyFlags && !isX) {
        auto& rigidbody = mSelf.get<RigidBody>();
        const bool wasGrounded = rigidbody.isGrounded;
        auto jumpControlOpt = mSelf.tryGet<Jumper>();
        const bool hasMomentum = isMomentumStored();
        Velocity& velocity = mSelf.get<Velocity>();

        if (hitinfo && hitinfo.isVertical()) {
            // update states for grounded, jumping, and reset impulses
            if (amount.y() <= 0 && hitinfo.isDown()) {
                rigidbody.setGrounded(hitinfo.otherMaterial);
            } else {
                rigidbody.setNotGrounded();
            }

            if (jumpControlOpt) {
                (*jumpControlOpt)->isJumping = false;
            }
            velocity.residualImpulse.e[1] = 0;
        } else if (!hitinfo) {
            rigidbody.setNotGrounded();
        }

        // UPDATE RIGIDBODY FLAGS, MOMENTUM, AND COYOTE TIME
        if (rigidbody.isGrounded) {
            rigidbody.isLanding = !wasGrounded;
            if (rigidbody.isLanding) {
                System::eventMgr.triggerEvent<LandingEvent>(mSelf);
                rigidbody.framesSinceLanding = 0;
            }

            if (hasMomentum) {
                onMomentumNotUsed();
            }

            if (velocity.total.y() < 0 && wasGrounded && (!jumpControlOpt || !(*jumpControlOpt)->isJumping)) {
                // zero y velocity when grounded and not trying to jump, otherwise entity falls at terminal velocity after walking off platform
                // do this on second frame on the ground
                velocity.stable.e[1] = 0;
                skipBounceStep = true;
            }

        } else {
            if (jumpControlOpt) {
                if (wasGrounded && !(*jumpControlOpt)->isJumping) {
                    (*jumpControlOpt)->coyoteSecondsRemaining = (*jumpControlOpt)->coyoteTimeSecondsMax;
                } else if ((*jumpControlOpt)->coyoteSecondsRemaining > 0) {
                    // is jumping
                    (*jumpControlOpt)->coyoteSecondsRemaining -= System::dt();
                    skipBounceStep = true;
                }
            }

            // prevent repeated push forces from accumulating huge speed
            if (hasMomentum && rigidbody.momentumCooldownFrames <= 0) {
                // convert to texels/sec
                velocity.stable += getMomentum() * FTEXELS_PER_PIXEL;
                resetMomentum();
                rigidbody.momentumCooldownFrames = MOMENTUM_COOLDOWN_FRAMES;
            } else if (rigidbody.momentumCooldownFrames > 0) {
                rigidbody.momentumCooldownFrames--;
            }
        }
    }

    if (hitinfo) {
        // BOUNCING
        // RESEARCH - using relative velocity instead of the mover's velocity would also be more accurate

        // average bounciness of both colliders
        f32 bounciness = (WhalMaterial::bounciness(mMaterial) + WhalMaterial::bounciness(hitinfo.otherMaterial)) / 2.0f;
        if (!skipBounceStep && bounciness != 0.0 && mSelf.has<Velocity>()) {
            auto& velocity = mSelf.get<Velocity>();
            if ((isX && abs(velocity.total.x()) >= BOUNCE_THRESHOLD) || (!isX && abs(velocity.total.y()) >= BOUNCE_THRESHOLD)) {
                s32 ix = isX ? 0 : 1;
                // stable can be negative (like for gravity) when impulse makes total velocity positive.
                // in that case we don't want to do anything
                if (sign(velocity.stable.e[ix]) == sign(velocity.total.e[ix])) {
                    velocity.stable.e[ix] = velocity.stable.e[ix] * -bounciness;
                }

                // do a post check in case an external force like gravity makes the first check always pass
                if (abs(velocity.stable.e[ix]) < BOUNCE_THRESHOLD) {
                    velocity.stable.e[ix] = 0;
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
    - bool isManualMove: should be true if this is called outside of the physics system. Only affects momentum of pushed/carried entities
*/
bool Collider::move(const Vector2f amount, const CollisionCallback callback, bool isGroundedCheckNeeded, bool isManualMove, bool isPushedBySolid,
                    bool updateRigidBodyFlags) {
    // round to nearest pixel
    mXRemainder += amount.x();
    mYRemainder += amount.y();

    Vector2i toMoveRounded = Vector2i(std::round(mXRemainder), std::round(mYRemainder));
    // only return early if we don't need a grounded check (solids can never be grounded)
    if (toMoveRounded.x() == 0 && toMoveRounded.y() == 0 && (!isGroundedCheckNeeded || isSolid())) {
        return false;
    }
    mXRemainder -= toMoveRounded.x();
    mYRemainder -= toMoveRounded.y();

    bool isHit = false;
    switch (mCollisionLayer) {
    case CollisionLayer::Actor: {
        isHit = emitCollisionInfo(amount, moveX(amount, toMoveRounded, callback), true, false);
        isHit = emitCollisionInfo(amount, moveY(amount, toMoveRounded, callback, isGroundedCheckNeeded), false, updateRigidBodyFlags) || isHit;
        break;
    }
    case CollisionLayer::Solid: {
        // check riding status *before* moving
        const auto riding = getRidingCollidersQT();

        // nothing can stop solids, so do full movement immediately and emit nothing
        // need to move+push on one axis before moving on the other
        auto moveVec = Vector2i(toMoveRounded.x(), 0);
        moveNoCollisionCheck(amount, moveVec);
        pushAndCarry1D(amount, moveVec, riding, isManualMove);

        moveVec = Vector2i(0, toMoveRounded.y());
        moveNoCollisionCheck(amount, moveVec);
        pushAndCarry1D(amount, moveVec, riding, isManualMove);
        break;
    }
    case CollisionLayer::SemiSolid: {
        // check riding status *before* moving
        const auto riding = getRidingCollidersQT();

        // moveX, then push/carry in that direction only
        auto originalPosition = getShape().getPosition();
        isHit = emitCollisionInfo(amount, moveX(amount, toMoveRounded, callback), true, false);
        Vector2i moveAmount = getShape().getPosition() - originalPosition;
        Vector2f moveUnrounded = isHit ? toFloatVec(moveAmount) : Vector2f(amount.x(), 0);
        pushAndCarry1D(moveUnrounded, moveAmount, riding, isManualMove, isPushedBySolid);

        // moveY, then push/carry in that direction only
        originalPosition = getShape().getPosition();
        const bool isHitY = emitCollisionInfo(amount, moveY(amount, toMoveRounded, callback, isGroundedCheckNeeded), false, updateRigidBodyFlags);
        isHit = isHit || isHitY;
        moveAmount = getShape().getPosition() - originalPosition;
        moveUnrounded = isHitY ? toFloatVec(moveAmount) : Vector2f(0, amount.y());
        pushAndCarry1D(moveUnrounded, moveAmount, riding, isManualMove, isPushedBySolid);

        break;
    }
    default:
        moveNoCollisionCheck(amount, toMoveRounded);
    }

    if (isManualMove) {
        // if we moved outside of the physics system we can update other components immediately
        updateEntityPosition();
    }

    return isHit;
}

HitInfo Collider::moveX(const Vector2f amount, const Vector2i amountRounded, const CollisionCallback callback) {
    s32 toMove = amountRounded.x();

    if (toMove == 0) {
        return HitInfo();
    }

    auto const canStopMe = getCollisionLayersThatCanStopMe();

    const s32 moveSign = sign(toMove);
    const auto moveNormal = Vector2i(moveSign, 0);
    while (toMove != 0) {
        auto nextPos = mShape.getPosition() + moveNormal;
        auto hitInfo = checkCollisionQT(nextPos, moveNormal, canStopMe, true);

        if (!hitInfo) {
            QuadTreeSystem::updatePosition(mSelf, &mShape, nextPos);
            toMove -= moveSign;
        } else {
            if (auto wiggleOpt = mSelf.tryGet<Wiggle>(); wiggleOpt) {
                if ((*wiggleOpt)->callback(this, hitInfo, moveNormal, amount)) {
                    continue;
                }
            }
            if (callback != nullptr) {
                callback(getEntity(), hitInfo.getOther(), moveNormal);
            }
            return hitInfo;
        }
    }
    return HitInfo();
}

HitInfo Collider::moveY(const Vector2f amount, const Vector2i amountRounded, const CollisionCallback callback, bool isGroundedCheckNeeded) {
    // include fractional movement from previous calls
    s32 toMove = amountRounded.y();

    auto groundedCheck = [this](f32 amountY) -> HitInfo {
        if (amountY > 0) {
            return HitInfo();
        }

        return checkIsGroundedQT(true);
    };

    if (toMove == 0) {
        if (isGroundedCheckNeeded) {
            return groundedCheck(amount.y());
        }
        return HitInfo();
    }

    auto const canStopMe = getCollisionLayersThatCanStopMe();

    const s32 moveSign = sign(toMove);
    const auto moveNormal = Vector2i(0, moveSign);
    while (toMove != 0) {
        auto nextPos = mShape.getPosition() + moveNormal;
        auto hitInfo = checkCollisionQT(nextPos, moveNormal, canStopMe, true);

        if (!hitInfo) {
            QuadTreeSystem::updatePosition(mSelf, &mShape, nextPos);
            toMove -= moveSign;
        } else {
            if (auto wiggleOpt = mSelf.tryGet<Wiggle>(); wiggleOpt) {
                if ((*wiggleOpt)->callback(this, hitInfo, moveNormal, amount)) {
                    continue;
                }
            }
            if (callback != nullptr) {
                callback(getEntity(), hitInfo.getOther(), moveNormal);
            }
            return hitInfo;
        }
    }

    if (isGroundedCheckNeeded) {
        return groundedCheck(amount.y());
    }
    return HitInfo();
}

void Collider::moveNoCollisionCheck(Vector2f toMove, Vector2i toMoveRounded) {
    QuadTreeSystem::updatePosition(mSelf, &mShape, mShape.getPosition() + toMoveRounded);
}

void Collider::pushAndCarry1D(Vector2f moveOriginal, Vector2i move1D, const std::vector<Collider*>& ridingColliders, bool isManualMove,
                              bool isPushedBySolid) {
    // turn off collision so colliders moved by us don't get stuck on us
    bool wasCollidable = mIsCollidable;
    mIsCollidable = false;

    // Caller should only have moved on one dimension before calling this, so only push/carry on that dimension
    if (move1D.x() > 0) {
        _pushAndCarry(move1D.x(), moveOriginal.x(), true, mShape.right(), &AABB::left, ridingColliders, isManualMove, isPushedBySolid);
    } else if (move1D.x() < 0) {
        _pushAndCarry(move1D.x(), moveOriginal.x(), true, mShape.left(), &AABB::right, ridingColliders, isManualMove, isPushedBySolid);
    } else if (move1D.y() > 0) {
        _pushAndCarry(move1D.y(), moveOriginal.y(), false, mShape.top(), &AABB::bottom, ridingColliders, isManualMove, isPushedBySolid);
    } else if (move1D.y() < 0) {
        _pushAndCarry(move1D.y(), moveOriginal.y(), false, mShape.bottom(), &AABB::top, ridingColliders, isManualMove, isPushedBySolid);
    }

    mIsCollidable = wasCollidable;
}

// we are moving, other is still.
bool Collider::isCollisionPossible(const Collider* other, const Vector2i moveNormal, const u16 layerMask) const {
    return other->mIsCollidable && this != other && LAYER_MATRIX.isOn(mCollisionLayer, other->mCollisionLayer) &&
           (layerMask & other->mCollisionLayer) > 0 && checkDirectionalCollision(mShape, other->mShape, moveNormal, other->getCollisionDir());
}

// other is moving, we are still. Only affects directional collision check.
bool Collider::isCollisionPossibleReversed(const Collider* other, const Vector2i moveNormal, const u16 layerMask) const {
    return other->mIsCollidable && this != other && LAYER_MATRIX.isOn(mCollisionLayer, other->mCollisionLayer) &&
           (layerMask & other->mCollisionLayer) > 0 && checkDirectionalCollision(other->mShape, mShape, moveNormal, getCollisionDir());
}

// Check for collision 1 unit down.
// (making sure to use the unmoved collider for the directional collision check so the edges are properly aligned)
HitInfo Collider::checkIsGroundedQT(const bool triggerCollisionEvents) {
    const auto movedCollider = AABB(mShape.getPosition() + Vector2i::unitDown, mShape.getHalf());
    HitInfo hitinfo;

    for (auto entity : QuadTreeSystem::query(movedCollider)) {
        const auto pCollider = &entity.get<Collider>();
        if (isCollisionPossible(pCollider, {0, -1}) && isOtherGround(pCollider)) {
            hitinfo = HitInfo(Vector2i(0, -1));
            hitinfo.setOther(pCollider->getEntity());
            hitinfo.otherMaterial = pCollider->getMaterial();
            hitinfo.otherLayer = pCollider->getCollisionLayer();

            if (triggerCollisionEvents) {
                System::eventMgr.triggerEvent<CollisionEvent>(mSelf, hitinfo);
            }
        }
    }

    return hitinfo;
}

// check other's collision layer and direction.
bool Collider::isOtherGround(const Collider* other) const {
    // semisolids should be ground for each other, so just check if other is any solid? Works ig.

    // return (other->mCollisionLayer & getCollisionLayersThatCanStopMe()) > 0 &&
    return (other->isSolidAny() && (other->mCollisionDir == CollisionDir::ALL || other->mCollisionDir == CollisionDir::UP));
}

// get colliders that are riding us. Basically check which colliders would intersect us if we moved 1px up, taking collision layers and directional
// collision into account.
std::vector<Collider*> Collider::getRidingCollidersQT() const {
    std::vector<Collider*> riding;
    const auto movedCollider = AABB(mShape.getPosition() + Vector2i::unitUp, mShape.getHalf());
    const auto layerMask = getCollisionLayersThatCanRideMe();

    for (auto entity : QuadTreeSystem::query(movedCollider)) {
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
    // ALL as default is fine.
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

void Collider::_pushAndCarry(s32 toMoveRounded, f32 toMoveUnrounded, bool isXDirection, s32 solidEdge, EdgeGetter edgeFunc,
                             const std::vector<Collider*>& riding, bool isManualMove, bool isPushedBySolid) {
    Vector2i moveVec;
    if (isXDirection) {
        moveVec = {toMoveRounded, 0};
    } else {
        moveVec = {0, toMoveRounded};
    }
    const auto prevColliderPos = AABB(mShape.getPosition() - moveVec, mShape.getHalf());
    const auto prevColliderState = Collider(prevColliderPos, mCollisionLayer, mMaterial, nullptr, mCollisionDir);

    if (abs(static_cast<f32>(toMoveRounded) - toMoveUnrounded) > 1) {
        print("Rounding anomaly: Unrounded move value is", toMoveUnrounded, "but rounded value is ", toMoveRounded);
    }

    const f32 dt = System::dt();
    std::vector<Collider*> toCarry = riding;
    const u16 notSolidMask = ~CollisionLayer::Solid;  // cannot be pushed or carried
    for (auto entity : QuadTreeSystem::query(mShape)) {
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
                hitSolid = other->move(toFloatVec(otherMoveVec), &squishEntityPushedBySemiSolid);

                Vector2i newPosition = other->getShape().getPosition();
                // Calculate difference between newPosition and expected position.
                // If we didn't hit something, but delta is nonzero, then something that `other` pushed hit a solid.
                auto delta = ((originalPosition + otherMoveVec) - newPosition) * -1;
                if (other->mIsAlive && other->isSemiSolid() && (hitSolid || delta.x() != 0 || delta.y() != 0)) {
                    // if other didn't move the full amount, it must have hit a solid, so push *this* back by the difference
                    // using &squishCollider as the callback because we're effectively being pushed by the solid that `other` hit
                    mIsCollidable = true;
                    other->mIsCollidable = false;
                    move(toFloatVec(delta), &squishEntity, false, false, true);
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
                        toMoveRounded = moveVec.x();
                        toMoveUnrounded += delta.x();
                    } else {
                        solidEdge = toMoveRounded > 0 ? mShape.top() : mShape.bottom();
                        toMoveRounded = moveVec.y();
                        toMoveUnrounded += delta.y();
                    }
                }
            } else {
                other->move(toFloatVec(otherMoveVec), &squishEntity, false, false, true);
            }

            // emit push event
            HitInfo hitinfo(moveVec, false, true);
            hitinfo.setOther(other->getEntity());
            hitinfo.otherMaterial = other->getMaterial();
            hitinfo.otherLayer = other->getCollisionLayer();
            System::eventMgr.triggerEvent<CollisionEvent>(mSelf, hitinfo);

            // set momentum if this movement was part of the physics system
            if (isManualMove) {
                other->maintainMomentum(isXDirection);
            } else {
                // don't worry about rounding, we're just moving the overlap distance
                f32 momentum = static_cast<f32>(toMoveRounded) / dt;
                other->setMomentum(momentum, isXDirection);
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
        HitInfo hitinfo(moveVec, false, false, true);
        hitinfo.setOther(other->getEntity());
        hitinfo.otherMaterial = other->getMaterial();
        hitinfo.otherLayer = other->getCollisionLayer();
        System::eventMgr.triggerEvent<CollisionEvent>(mSelf, hitinfo);

        // set momentum if this movement was part of the physics system
        if (isManualMove) {
            other->maintainMomentum(isXDirection);
        } else {
            f32 momentum = toMoveUnrounded / dt;
            other->setMomentum(momentum, isXDirection);
        }
    }
}

HitInfo Collider::checkCollisionQT(const Vector2i position, const Vector2i moveNormal, const u16 layerMask, const bool triggerCollisionEvents) const {
    if (!isCollidable()) {
        return HitInfo();
    }
    const auto movedCollider = AABB(position, mShape.getHalf());
    HitInfo hitInfoToReturn;  // used for updating rigidbody flags n such. doesn't matter which specific collision is returned.

    for (auto entity : QuadTreeSystem::query(movedCollider)) {
        const auto collider = entity.get<Collider>();
        if (!isCollisionPossible(&collider, moveNormal, layerMask)) {
            continue;
        }

        HitInfo hitInfo = movedCollider.collide(collider.getShape());
        if (hitInfo) {
            hitInfo.setOther(entity);
            hitInfo.otherLayer = collider.getCollisionLayer();
            hitInfo.otherMaterial = collider.getMaterial();

            // only care about the hit flag for the direction we're moving in (in the case of a corner hit)
            if (moveNormal.x() != 0) {
                hitInfo.clearVerticalFlags();
            } else {
                hitInfo.clearHorizontalFlags();
            }

            hitInfoToReturn = hitInfo;
            if (triggerCollisionEvents) {
                System::eventMgr.triggerEvent<CollisionEvent>(mSelf, hitInfo);
            }
        }
    }

    return hitInfoToReturn;
}

void Collider::squish(ecs::Entity other, Vector2i hitNormal) {
    mSquishCallback(mSelf, other, hitNormal);
}

void Collider::setMomentum(const f32 momentum, const bool isXDirection) {
    auto eRB = getEntity().tryGet<RigidBody>();
    if (!eRB) {
        return;
    }
    if (isXDirection) {
        mStoredMomentum.e[0] = momentum * (*eRB)->momentumMultiplier.x();
        mMomentumFramesLeft.e[0] = MOMENTUM_LIFETIME_FRAMES;
    } else {
        mStoredMomentum.e[1] = momentum * (*eRB)->momentumMultiplier.y();
        mMomentumFramesLeft.e[1] = MOMENTUM_LIFETIME_FRAMES;
    }
}

void Collider::maintainMomentum(const bool isXDirection) {
    if (isXDirection) {
        mMomentumFramesLeft.e[0] = MOMENTUM_LIFETIME_FRAMES;
    } else {
        mMomentumFramesLeft.e[1] = MOMENTUM_LIFETIME_FRAMES;
    }
}

void Collider::onMomentumNotUsed() {
    mMomentumFramesLeft -= {1, 1};
    if (!mMomentumFramesLeft.x()) {
        mStoredMomentum.e[0] = 0;
        mMomentumFramesLeft.e[0] = 0;
    }
    if (!mMomentumFramesLeft.y()) {
        mStoredMomentum.e[1] = 0;
        mMomentumFramesLeft.e[1] = 0;
    }
}

void Collider::resetMomentum() {
    mStoredMomentum = {0, 0};
    mMomentumFramesLeft = {0, 0};
}

// Try to wiggle out of collision if barely clipping another collider.
// Returns true if successful.
bool Collider::tryCornerCorrection(Vector2i nextPosition, s32 moveSignX, Vector2i moveNormal) {
    if (moveSignX >= 0) {
        // if we are on a half texel x coord, start at 0.5 texels of movement
        for (s32 i = PIXELS_PER_TEXEL - nextPosition.x() % PIXELS_PER_TEXEL; i <= CORNERCORRECTIONWIGGLE; i += PIXELS_PER_TEXEL) {
            Vector2i nextPos = nextPosition + Vector2i(i, 0);
            if (!checkCollisionQT(nextPos, moveNormal)) {
                QuadTreeSystem::updatePosition(mSelf, &mShape, nextPos);
                return true;
            }
        }
    }
    if (moveSignX <= 0) {
        // if we are on a half texel x coord, start at 0.5 texels of movement
        for (s32 i = PIXELS_PER_TEXEL - nextPosition.x() % PIXELS_PER_TEXEL; i <= CORNERCORRECTIONWIGGLE; i += PIXELS_PER_TEXEL) {
            Vector2i nextPos = nextPosition + Vector2i(-i, 0);
            if (!checkCollisionQT(nextPos, moveNormal)) {
                QuadTreeSystem::updatePosition(mSelf, &mShape, nextPos);
                return true;
            }
        }
    }
    return false;
}

}  // namespace whal

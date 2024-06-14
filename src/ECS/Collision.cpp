#include "Collision.h"

#include <algorithm>
#include <cmath>

#include "ECS/Name.h"
#include "ECS/PlayerControl.h"
#include "ECS/RigidBody.h"
#include "ECS/Systems/CollisionManager.h"
#include "ECS/Transform.h"
#include "ECS/Velocity.h"

#include "Game/Events.h"  // TODO using event for core logic, should bring those into engine
#include "Physics/CollisionLayer.h"
#include "Settings.h"

#include "Physics/HitInfo.h"
#include "Physics/IUseCollision.h"
#include "Systems/System.h"
#include "Util/MathUtil.h"
#include "Util/Print.h"

namespace whal {

// constexpr s32 MOMENTUM_LIFETIME_FRAMES = 10;
constexpr s32 CORNERCORRECTIONWIGGLE = 3 * PIXELS_PER_TEXEL;

// a semisolid pushing another semisolid shouldn't squish it. If it runs into a [semi]solid, just stop movement
void squishPushedBySemiSolid(ecs::Entity callbackEntity, ecs::Entity other, Collider* callbackEntityCollider, Collider* otherCollider,
                             Vector2i hitNormal) {
    if (callbackEntityCollider->isSemiSolid() && otherCollider->isSolidAny()) {
        return;
    }
    callbackEntityCollider->squish();
}

void squishCollider(ecs::Entity callbackEntity, ecs::Entity other, Collider* callbackEntityCollider, Collider* otherCollider, Vector2i hitNormal) {
    // TODO make squish method take all these args and move this logic there
    if (callbackEntityCollider->isSemiSolid() && otherCollider->isSemiSolid()) {
        return;
    }
    callbackEntityCollider->squish();
}

Collider::Collider(AABB shape, CollisionLayer::Layer layer, WorldMaterial material, CollisionCallbackNew onCollisionEnter_, CollisionDir collisionDir)
    : mShape(shape), mCollisionLayer(layer), mOnCollisionEnter(onCollisionEnter_), mMaterial(material), mCollisionDir(collisionDir) {}

Collider::Collider(Transform2D transform, Vector2i halflen, CollisionLayer::Layer layer, WorldMaterial material,
                   CollisionCallbackNew onCollisionEnter_, CollisionDir collisionDir)
    : mShape(AABB(transform, halflen)), mCollisionLayer(layer), mOnCollisionEnter(onCollisionEnter_), mMaterial(material),
      mCollisionDir(collisionDir) {}

Collider Collider::Actor(AABB shape) {
    return Collider(shape, CollisionLayer::Actor);
}

Collider Collider::Actor(Transform2D transform, Vector2i halflen) {
    return Collider(transform, halflen, CollisionLayer::Actor);
}

Collider Collider::Solid(AABB shape, WorldMaterial material, CollisionCallbackNew onCollisionEnter_, CollisionDir collisionDir) {
    return Collider(shape, CollisionLayer::Solid, material, onCollisionEnter_, collisionDir);
}

Collider Collider::Solid(Transform2D transform, Vector2i halflen, WorldMaterial material, CollisionCallbackNew onCollisionEnter_,
                         CollisionDir collisionDir) {
    return Collider(transform, halflen, CollisionLayer::Solid, material, onCollisionEnter_, collisionDir);
}

Collider Collider::SemiSolid(AABB shape, WorldMaterial material, CollisionCallbackNew onCollisionEnter_, CollisionDir collisionDir) {
    return Collider(shape, CollisionLayer::SemiSolid, material, onCollisionEnter_, collisionDir);
}

Collider Collider::SemiSolid(Transform2D transform, Vector2i halflen, WorldMaterial material, CollisionCallbackNew onCollisionEnter_,
                             CollisionDir collisionDir) {
    return Collider(transform, halflen, CollisionLayer::SemiSolid, material, onCollisionEnter_, collisionDir);
}

// dispatches correct move method based on Collision Layer
/* args
    - const Vector2f amount: float amount to move, in pixels
    - const CollisionCallbackNew callback: callback to run on *this* if a collision occurs
    - bool isGroundedCheckNeeded: skips grounded check if false
    - bool isManualMove: should be true if this is called outside of the physics system. Only affects momentum of pushed/carried entities
*/
bool Collider::move(const Vector2f amount, const CollisionCallbackNew callback, bool isGroundedCheckNeeded, bool isManualMove, bool isPushedBySolid) {
    // TODO this should be a class method
    const auto emitCollisionInfo = [=, this](HitInfo hitinfo, bool isX) -> bool {
        if (!isX && mSelf.has<RigidBody>()) {
            auto rigidbody = mSelf.get<RigidBody>();
            const bool wasGrounded = rigidbody.isGrounded;
            auto jumpControlOpt = mSelf.tryGet<Jumper>();
            // bool isMomentumStored = actor.value()->isMomentumStoredX() || actor.value()->isMomentumStoredY();

            if (hitinfo && hitinfo.isVertical()) {
                // update states for grounded, jumping, and reset impulses
                if (amount.y() <= 0 && hitinfo.isDown()) {
                    rigidbody.setGrounded(hitinfo.otherMaterial);
                } else {
                    rigidbody.setNotGrounded();
                }

                if (jumpControlOpt) {
                    jumpControlOpt.value()->isJumping = false;
                }
                mSelf.get<Velocity>().residualImpulse.e[1] = 0;
            } else if (!hitinfo) {
                rigidbody.setNotGrounded();
            }

            // UPDATE RIGIDBODY FLAGS, MOMENTUM, AND COYOTE TIME
            // should probably be a separate method?
            if (rigidbody.isGrounded) {
                rigidbody.isLanding = !wasGrounded;
                if (!wasGrounded) {
                    System::eventMgr.triggerEvent(Event::LANDING_EVENT, mSelf);
                }
                // TODO
                // if (isMomentumStored) {
                //     actor.value()->momentumNotUsed();
                // }

            } else {
                if (jumpControlOpt) {
                    if (wasGrounded && !jumpControlOpt.value()->isJumping) {
                        jumpControlOpt.value()->coyoteSecondsRemaining = jumpControlOpt.value()->coyoteTimeSecondsMax;
                    } else if (jumpControlOpt.value()->coyoteSecondsRemaining > 0) {
                        jumpControlOpt.value()->coyoteSecondsRemaining -= System::dt();
                    }
                }

                // TODO
                // prevent repeated push forces from accumulating huge speed
                // if (isMomentumStored && rb.value()->momentumCooldownFrames <= 0) {
                //     // convert to texels/sec
                //     vel.stable += actor.value()->getMomentum() * FTEXELS_PER_PIXEL;
                //     actor.value()->resetMomentum();
                //     rb.value()->momentumCooldownFrames = MOMENTUM_COOLDOWN_FRAMES;
                // } else if (rb.value()->momentumCooldownFrames > 0) {
                //     rb.value()->momentumCooldownFrames--;
                // }
            }

            mSelf.set(rigidbody);
        }

        if (hitinfo) {
            System::eventMgr.triggerEvent(Event::COLLISION_EVENT, mSelf, hitinfo);
            return true;
        }
        return false;
    };

    // round to nearest pixel
    mXRemainder += amount.x();
    mYRemainder += amount.y();

    Vector2i toMoveRounded = Vector2i(std::round(mXRemainder), std::round(mYRemainder));
    // only return early if we don't need a grounded check
    if (toMoveRounded.x() == 0 && toMoveRounded.y() == 0 && (!isGroundedCheckNeeded || isSolid())) {
        return false;
    }
    mXRemainder -= toMoveRounded.x();
    mYRemainder -= toMoveRounded.y();

    bool isHit = false;
    switch (mCollisionLayer) {
    case CollisionLayer::Actor: {
        isHit = emitCollisionInfo(moveX(amount, toMoveRounded, callback), true);
        isHit = emitCollisionInfo(moveY(amount, toMoveRounded, callback, isGroundedCheckNeeded), false) || isHit;
        break;
    }
    case CollisionLayer::Solid: {
        // check riding status *before* moving
        const auto riding = getRidingColliders();

        // nothing can stop solids, so do full movement immediately and emit nothing
        // need to move+push on one axis before moving on the other
        auto moveVec = Vector2i(toMoveRounded.x(), 0);
        moveNoCollisionCheck(amount, moveVec);
        pushAndCarry1D(amount, moveVec, riding, isManualMove);  // TODO this still needs to trigger collision event

        moveVec = Vector2i(0, toMoveRounded.y());
        moveNoCollisionCheck(amount, moveVec);
        pushAndCarry1D(amount, moveVec, riding, isManualMove);  // TODO this still needs to trigger collision event
        break;
    }
    case CollisionLayer::SemiSolid: {
        // check riding status *before* moving
        const auto riding = getRidingColliders();
        std::string name = "entity";
        if (mSelf.has<Name>()) {
            name = mSelf.get<Name>().name;
        }

        print("");
        // moveX, then push/carry in that direction only
        auto originalPosition = getCollider().getPosition();
        // print("doing", name, "'s moveX. amount is ", toMoveRounded, "and position is ", mShape.getPosition());
        isHit = emitCollisionInfo(moveX(amount, toMoveRounded, callback), true);
        // print("after moveX, position is ", mShape.getPosition());
        Vector2i moveAmount = getCollider().getPosition() - originalPosition;
        pushAndCarry1D({amount.x(), 0}, moveAmount, riding, isManualMove, isPushedBySolid);
        // print("after pushAndCarryX, position is", mShape.getPosition());

        bool shouldPrint = toMoveRounded.x() != 0 || toMoveRounded.y() != 0;
        // moveY, then push/carry in that direction only
        originalPosition = getCollider().getPosition();
        if (shouldPrint)
            print("doing", name, "'s moveY. amount is ", toMoveRounded, "and position is ", mShape.getPosition());
        isHit = emitCollisionInfo(moveY(amount, toMoveRounded, callback, isGroundedCheckNeeded), false) || isHit;
        if (shouldPrint)
            print("after moveY, position is ", mShape.getPosition());
        moveAmount = getCollider().getPosition() - originalPosition;
        if (shouldPrint)
            print("starting pushAndCarry for ", name, "amount is ", moveAmount);
        pushAndCarry1D({0, amount.y()}, moveAmount, riding, isManualMove, isPushedBySolid);
        if (shouldPrint)
            print("after pushAndCarryY,", name, "'s position is", mShape.getPosition());
        if (shouldPrint)
            print("");

        break;
    }
    default:
        moveNoCollisionCheck(amount, toMoveRounded);
    }

    return isHit;
}

HitInfo Collider::moveX(const Vector2f amount, const Vector2i amountRounded, const CollisionCallbackNew callback) {
    s32 toMove = amountRounded.x();

    if (toMove == 0) {
        return HitInfo();
    }

    auto const& others = CollisionManager::instance()->getPhysicsColliders();
    auto const canStopMe = getCollisionLayersThatCanStopMe();

    const s32 moveSign = sign(toMove);
    const auto moveNormal = Vector2i(moveSign, 0);
    while (toMove != 0) {
        auto nextPos = mShape.getPosition() + moveNormal;
        auto hitInfo = checkCollision(others, nextPos, moveNormal, canStopMe);

        if (!hitInfo) {
            mShape.setPosition(nextPos);
            toMove -= moveSign;
        } else {
            if (callback != nullptr) {
                callback(getEntity(), hitInfo.other, this, &hitInfo.other.get<Collider>(), moveNormal);
            }
            return hitInfo;
        }
    }
    return HitInfo();
}

HitInfo Collider::moveY(const Vector2f amount, const Vector2i amountRounded, const CollisionCallbackNew callback, bool isGroundedCheckNeeded) {
    // include fractional movement from previous calls
    s32 toMove = amountRounded.y();
    auto const& others = CollisionManager::instance()->getPhysicsColliders();

    auto groundedCheck = [this, others](f32 amountY) -> HitInfo {
        Collider* groundCollider = nullptr;
        if (amountY > 0) {
            return HitInfo();
        }

        if (checkIsGrounded(others, &groundCollider)) {
            HitInfo hitinfo(Vector2i(0, -1));
            hitinfo.other = groundCollider->getEntity();
            hitinfo.otherMaterial = groundCollider->getMaterial();
            hitinfo.otherLayer = groundCollider->mCollisionLayer;
            return hitinfo;
        }

        return HitInfo();
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
        auto hitInfo = checkCollision(others, nextPos, moveNormal, canStopMe);

        if (!hitInfo) {
            mShape.setPosition(nextPos);
            toMove -= moveSign;
        } else {
            // if actor and other is [semi]solid then try corner correction
            // should make this its own method
            if (moveSign == 1 && (hitInfo.otherLayer & (CollisionLayer::Solid | CollisionLayer::SemiSolid)) > 0 &&
                mCollisionLayer == CollisionLayer::Actor) {
                // TODO wiggling out of the way should be done in squish, so it only happens when we want it to. ALso squish should be configurable
                // and I guess return a bool in case the collision was avoided? Idk maybe it's fine for the corner correction to eat the movement that
                // would've happened
                if (tryCornerCorrection(others, nextPos, amount.x(), moveNormal)) {
                    continue;  // avoided collision
                }
            }
            if (callback != nullptr) {
                callback(getEntity(), hitInfo.other, this, &hitInfo.other.get<Collider>(), moveNormal);
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
    mShape.setPosition(mShape.getPosition() + toMoveRounded);
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

bool Collider::checkIsGrounded(const std::vector<Collider*>& otherColliders, Collider** dstGroundCollider) {
    // RESEARCH collisionManager could track which map tiles are ground to reduce checks here?
    for (auto pCollider : otherColliders) {
        if (pCollider->isCollidable() && pCollider != this && LAYER_MATRIX.isOn(mCollisionLayer, pCollider->mCollisionLayer) &&
            pCollider->isGround() && isRiding(pCollider)) {
            *dstGroundCollider = pCollider;
            return true;
        }
    }
    return false;
}

// checks if collider has upwards collision & is a [semi]solid
bool Collider::isGround() const {
    return (mCollisionDir == CollisionDir::ALL || mCollisionDir == CollisionDir::UP) &&
           (mCollisionLayer & (CollisionLayer::SemiSolid | CollisionLayer::Solid)) > 0;
}

// Check for collision 1 unit down.
// (making sure to use the unmoved collider for the directional collision check so the edges are properly aligned)
// ASSUMES THAT COLLISION LAYERS HAVE BEEN CHECKED + BOTH COLLIDERS ARE COLLIDABLE AND THE POINTERS ARE NOT EQUAL
bool Collider::isRiding(const Collider* other) const {
    auto movedCollider = AABB(mShape.getPosition() + Vector2i::unitDown, mShape.getHalf());
    if (movedCollider.isOverlapping(&other->getCollider()) &&
        (other->getCollisionDir() == CollisionDir::ALL ||
         checkDirectionalCollision(getCollider(), other->getCollider(), {0, -1}, other->getCollisionDir()))) {
        return true;
    }
    return false;
}

std::vector<Collider*> Collider::getRidingColliders() const {
    std::vector<Collider*> riding;
    for (auto pCollider : CollisionManager::instance()->getPhysicsColliders()) {
        if (pCollider->isCollidable() && pCollider != this && LAYER_MATRIX.isOn(mCollisionLayer, pCollider->mCollisionLayer) &&
            pCollider->isRiding(this)) {
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
    // TODO when debugging, saw toMoveRounded = -3, toMoveUnrounded = -4.11 ???

    auto mask = getCollisionLayersThatCanRideMe();
    for (auto other : CollisionManager::instance()->getPhysicsColliders()) {
        if (!LAYER_MATRIX.isOn(mCollisionLayer, other->mCollisionLayer) || (other->mCollisionLayer & mask) == 0 || !other->isCollidable() ||
            other == this) {
            continue;
        }
        // push takes priority over carry
        if (mShape.isOverlapping(&other->getCollider()) &&
            checkDirectionalCollision(other->getCollider(), prevColliderPos, moveVec * -1, getCollisionDir())) {
            s32 actorEdge = (other->getCollider().*edgeFunc)();
            s32 toMoveOverlap = solidEdge - actorEdge;
            Vector2i otherMoveVec = isXDirection ? Vector2i(toMoveOverlap, 0) : Vector2i(0, toMoveOverlap);

            // If we are a semisolid pushing another semisolid, and a solid is not pushing us, then `other` may "push back" on us.
            // If a solid is pushing us though, then `other` is effectively being pushed by a solid.
            if (!isPushedBySolid && isSemiSolid()) {
                Vector2i originalPosition = other->getCollider().getPosition();
                bool hitSolid = false;
                std::string otherName = "other";
                if (other->mSelf.has<Name>()) {
                    otherName = other->mSelf.get<Name>().name;
                }
                print("pushing", otherName, "by", otherMoveVec);
                hitSolid = other->move(toFloatVec(otherMoveVec), &squishPushedBySemiSolid);

                Vector2i newPosition = other->getCollider().getPosition();
                // Calculate difference between newPosition and expected position.
                // If we didn't hit something, but delta is nonzero, then something that `other` pushed hit a solid.
                auto delta = ((originalPosition + otherMoveVec) - newPosition) * -1;
                if (other->mIsAlive && other->isSemiSolid() && (hitSolid || delta.x() != 0 || delta.y() != 0)) {
                    // if other didn't move the full amount, it must have hit a solid, so push *this* back by the difference
                    // using &squishCollider as the callback because we're effectively being pushed by the solid that `other` hit
                    mIsCollidable = true;
                    other->mIsCollidable = false;
                    std::string name = "entity";
                    if (mSelf.has<Name>()) {
                        name = mSelf.get<Name>().name;
                    }
                    print(name, "being pushed back by", otherName, ". Its position is", mShape.getPosition(), "and it's being pushed back by", delta);
                    move(toFloatVec(delta), &squishCollider, false, false, true);
                    print("After being pushed back,", name, "'s position is ", mShape.getPosition());
                    mIsCollidable = false;
                    other->mIsCollidable = true;

                    if (!mIsAlive) {
                        print("push back killed", name);
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
                other->move(toFloatVec(otherMoveVec), &squishCollider, false, false, true);
            }
            // TODO
            // if (isManualMove) {
            //     other->maintainMomentum(isXDirection);
            // } else {
            //     // don't worry about rounding, we're just moving the overlap distance
            //     f32 momentum = static_cast<f32>(toMoveRounded) / dt;
            //     other->setMomentum(momentum, isXDirection);
            // }
        } else if (std::find(riding.begin(), riding.end(), other) != riding.end()) {
            // I might change this for solids moving down faster than gravity RESEARCH
            if (isXDirection) {
                other->move(Vector2f(toMoveRounded, 0), nullptr);
            } else {
                other->move(Vector2f(0, toMoveRounded), nullptr);
            }
            // TODO
            // if (isManualMove) {
            //     other->maintainMomentum(isXDirection);
            // } else {
            //     f32 momentum = toMoveUnrounded / dt;
            //     other->setMomentum(momentum, isXDirection);
            // }
        }
    }
}

HitInfo Collider::checkCollision(const std::vector<Collider*>& colliders, const Vector2i position, const Vector2i moveNormal,
                                 const u16 layerMask) const {
    if (!isCollidable()) {
        return HitInfo();
    }
    const auto movedCollider = AABB(position, mShape.getHalf());
    for (auto pCollider : colliders) {
        if (!pCollider->isCollidable() || this == pCollider || !LAYER_MATRIX.isOn(mCollisionLayer, pCollider->mCollisionLayer) ||
            !(pCollider->mCollisionLayer & layerMask)) {
            continue;
        }

        // check for one-way collision skips
        const CollisionDir collisionDir = pCollider->getCollisionDir();
        if (!checkDirectionalCollision(getCollider(), pCollider->getCollider(), moveNormal, collisionDir)) {
            continue;
        }

        HitInfo hitInfo = movedCollider.collide(pCollider->getCollider());
        if (hitInfo) {
            hitInfo.other = pCollider->getEntity();
            hitInfo.otherLayer = pCollider->mCollisionLayer;
            hitInfo.otherMaterial = pCollider->getMaterial();
            return hitInfo;
        }
    }

    return HitInfo();
}

// RESEARCH should probably have this call a function pointer class member
void Collider::squish() {
    mSelf.kill();
    mIsAlive = false;
}

// Try to wiggle out of collision if barely clipping another collider.
// Returns true if successful.
bool Collider::tryCornerCorrection(const std::vector<Collider*>& others, Vector2i nextPosition, s32 moveSignX, Vector2i moveNormal) {
    if (moveSignX >= 0) {
        // if we are on a half texel x coord, start at 0.5 texels of movement
        for (s32 i = PIXELS_PER_TEXEL - nextPosition.x() % PIXELS_PER_TEXEL; i <= CORNERCORRECTIONWIGGLE; i += PIXELS_PER_TEXEL) {
            Vector2i nextPos = nextPosition + Vector2i(i, 0);
            if (!checkCollision(others, nextPos, moveNormal)) {
                mShape.setPosition(nextPos);
                return true;
            }
        }
    }
    if (moveSignX <= 0) {
        // if we are on a half texel x coord, start at 0.5 texels of movement
        for (s32 i = PIXELS_PER_TEXEL - nextPosition.x() % PIXELS_PER_TEXEL; i <= CORNERCORRECTIONWIGGLE; i += PIXELS_PER_TEXEL) {
            Vector2i nextPos = nextPosition + Vector2i(-i, 0);
            if (!checkCollision(others, nextPos, moveNormal)) {
                mShape.setPosition(nextPos);
                return true;
            }
        }
    }
    return false;
}

}  // namespace whal

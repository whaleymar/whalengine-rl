#pragma once

#include <vector>

#include "Physics/CollisionLayer.h"
#include "Physics/CollisionUtil.h"
#include "Physics/Material.h"
#include "Physics/Shapes.h"
#include "Util/Vector.h"
#include "whalECS/src/ECS.h"

namespace whal {

class Collider;
struct HitInfo;
class PhysicsSystem;
class TweenPositionSystem;
// namespace ecs {
// class Entity;
// }

// the default function which is called when a non-solid collider is squished between two solids (it dies).
void defaultSquish(ecs::Entity callbackEntity, ecs::Entity other, Vector2i hitNormal);

// currently 64 bytes, don't want to make it bigger for cache reasons
class Collider {
    friend PhysicsSystem;
    friend TweenPositionSystem;

public:
    Collider() = default;
    Collider(AABB shape, CollisionLayer::Layer layer, WorldMaterial material = WorldMaterial::None, CollisionCallback onCollisionEnter_ = nullptr,
             CollisionDir collisionDir = CollisionDir::ALL, CollisionCallback squish_ = &defaultSquish);
    Collider(Transform2D transform, Vector2i halflen, CollisionLayer::Layer layer, WorldMaterial material = WorldMaterial::None,
             CollisionCallback onCollisionEnter_ = nullptr, CollisionDir collisionDir = CollisionDir::ALL,
             CollisionCallback squish_ = &defaultSquish);

    // static creator functions
    static Collider Actor(AABB shape, CollisionCallback squish_ = &defaultSquish);
    static Collider Actor(Transform2D transform, Vector2i halflen, CollisionCallback squish_ = &defaultSquish);
    static Collider Solid(AABB shape, WorldMaterial material = WorldMaterial::None, CollisionCallback onCollisionEnter_ = nullptr,
                          CollisionDir collisionDir = CollisionDir::ALL, CollisionCallback squish_ = &defaultSquish);
    static Collider Solid(Transform2D transform, Vector2i halflen, WorldMaterial material = WorldMaterial::None,
                          CollisionCallback onCollisionEnter_ = nullptr, CollisionDir collisionDir = CollisionDir::ALL,
                          CollisionCallback squish_ = &defaultSquish);
    static Collider SemiSolid(AABB shape, WorldMaterial material = WorldMaterial::None, CollisionCallback onCollisionEnter_ = nullptr,
                              CollisionDir collisionDir = CollisionDir::ALL, CollisionCallback squish_ = &defaultSquish);
    static Collider SemiSolid(Transform2D transform, Vector2i halflen, WorldMaterial material = WorldMaterial::None,
                              CollisionCallback onCollisionEnter_ = nullptr, CollisionDir collisionDir = CollisionDir::ALL,
                              CollisionCallback squish_ = &defaultSquish);

    const AABB& getShape() const { return mShape; }
    AABB& getShapeMutable() { return mShape; }
    void setShape(AABB shape) { mShape = shape; }
    CollisionCallback getOnCollisionEnter() const { return mOnCollisionEnter; }
    void setCollisionCallback(CollisionCallback callback);  // Sends update signal to CollisionManager if callback was previously null.
    void setSquishCallback(CollisionCallback callback) { mSquishCallback = callback; }
    WorldMaterial getMaterial() const { return mMaterial; }
    void setMaterial(WorldMaterial material) { mMaterial = material; }
    bool isAlive() const { return mIsAlive; }
    void setIsDead() { mIsAlive = false; }
    bool isCollidable() const { return mIsCollidable; }
    void setIsCollidable(bool isCollidable) { mIsCollidable = isCollidable; }
    CollisionDir getCollisionDir() const { return mCollisionDir; }
    void setCollisionDir(CollisionDir dir) { mCollisionDir = dir; }
    CollisionLayer::Layer getCollisionLayer() const { return mCollisionLayer; }
    void setCollisionLayer(CollisionLayer::Layer layer) { mCollisionLayer = layer; }

    ecs::Entity getEntity() const { return mSelf; }
    void setEntity(ecs::Entity entity) { mSelf = entity; }

    bool isActor() const { return mCollisionLayer & CollisionLayer::Actor; }
    bool isSolid() const { return mCollisionLayer & CollisionLayer::Solid; }
    bool isSemiSolid() const { return mCollisionLayer & CollisionLayer::SemiSolid; }
    bool isSolidAny() const { return LAYER_MATRIX.isSolidAny(mCollisionLayer); }

    bool move(const Vector2f amount, const CollisionCallback callback, bool isGroundedCheckNeeded = false, bool isManualMove = false,
              bool isPushedBySolid = false, bool updateRigidBodyFlags = false, bool isSkipMomentumUpdate = false);
    HitInfo moveX(const Vector2f amountOriginal, const Vector2i amountRounded, const CollisionCallback callback,
                  const std::vector<std::pair<ecs::Entity, Collider>>& others);
    HitInfo moveY(const Vector2f amountOriginal, const Vector2i amountRounded, const CollisionCallback callback,
                  const std::vector<std::pair<ecs::Entity, Collider>>& others, bool isGroundedCheckNeeded = false);

    void moveNoCollisionCheck(Vector2f toMoveOriginal, Vector2i toMoveRounded);
    void pushAndCarry1D(Vector2f moveOriginal, Vector2i move1D, const std::vector<Collider*>& ridingColliders, bool isManualMove = false,
                        bool isPushedBySolid = false, bool isSkipMomentumUpdate = false);
    bool emitCollisionInfo(const Vector2f amount, const HitInfo hitinfo, bool isXDirection, bool updateRigidBodyFlags);

    bool isCollisionPossible(const Collider* other, const Vector2i moveNormal, const u16 layerMask = CollisionLayer::ALL) const;
    bool isCollisionPossibleReversed(const Collider* other, const Vector2i moveNormal, const u16 layerMask = CollisionLayer::ALL) const;
    HitInfo checkIsGrounded(const bool triggerCollisionEvents, const std::vector<std::pair<ecs::Entity, Collider>>& groundColliders);
    bool isOtherGround(const Collider& other) const;
    std::vector<Collider*> getRidingCollidersQT() const;
    u16 getCollisionLayersThatCanStopMe() const;  // is this name specific enough?
    u16 getCollisionLayersThatCanRideMe() const;

    HitInfo checkCollisionQT(const Vector2i position, const Vector2i moveNormal, const u16 layerMask = CollisionLayer::ALL,
                             const bool triggerCollisionEvents = false) const;
    std::vector<std::pair<ecs::Entity, Collider>> getCollidersInMoveArea(const Vector2i toMove, const u16 layerMask = CollisionLayer::ALL,
                                                                         bool updateRigidBodyFlags = false) const;
    void squish(ecs::Entity other, Vector2i hitNormal);
    bool tryCornerCorrection(Vector2i nextPos, s32 moveSignX, Vector2i moveNormal);

protected:
    void updateEntityPosition();
    void _pushAndCarry(s32 toMoveRounded, f32 toMoveUnrounded, bool isXDirection, s32 solidEdge, EdgeGetter edgeFunc,
                       const std::vector<Collider*>& riding, bool isManualMove, bool isPushedBySolid, bool isSkipMomentumUpdate);
    HitInfo checkCollisionInMoveArea(const Vector2i position, const Vector2i moveNormal, const std::vector<std::pair<ecs::Entity, Collider>>& others,
                                     const bool triggerCollisionEvents = false) const;

    // Shape mShape; // Maybe one day. Too much is hard coded to AABBs for me to bother rn
    AABB mShape;
    ecs::Entity mSelf;
    CollisionLayer::Layer mCollisionLayer;
    bool mIsCollidable = true;
    bool mIsAlive = true;
    CollisionCallback mOnCollisionEnter = nullptr;
    CollisionCallback mSquishCallback = &defaultSquish;
    f32 mXRemainder = 0.0;
    f32 mYRemainder = 0.0;
    WorldMaterial mMaterial;
    CollisionDir mCollisionDir;
};

using WiggleCallback = bool (*)(Collider* callbackCollider, HitInfo hitinfo, Vector2i moveNormal, Vector2f fullMoveAmount);

bool defaultWiggle(Collider* callbackCollider, HitInfo hitinfo, Vector2i moveNormal, Vector2f fullMoveAmount);
struct Wiggle {
    WiggleCallback callback = &defaultWiggle;
};

// since movement is pixel perfect, rounding can have big effect on momentum
// so track the previous 5 momentum values and use their average
struct Momentum {
    static inline constexpr s32 MOMENTUM_STORAGE_COUNT = 5;

    Vector2T<s16> momentumFramesLeft;
    Vector2T<s16> nextIx;
    Vector2f storedMomentum[MOMENTUM_STORAGE_COUNT];
    s32 cooldownFrames = 0;

    void setMomentumX(ecs::Entity self, const f32 momentumX);
    void setMomentumY(ecs::Entity self, const f32 momentumY);

    void maintainMomentumX();
    void maintainMomentumY();

    bool isMomentumStored() const { return isMomentumStoredX() || isMomentumStoredY(); }
    bool isMomentumStoredX() const { return momentumFramesLeft.x > 0; }
    bool isMomentumStoredY() const { return momentumFramesLeft.y > 0; }

    void onMomentumNotUsed();

    void resetMomentum() {
        resetMomentumX();
        resetMomentumY();
    }
    void resetMomentumX();
    void resetMomentumY();

    Vector2f getMomentum() const;
};

struct ColliderOffset {
    Vector2i offset;
};

}  // namespace whal

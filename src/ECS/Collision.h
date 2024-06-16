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
// namespace ecs {
// class Entity;
// }

class Collider {
public:
    Collider() = default;
    Collider(AABB shape, CollisionLayer::Layer layer, WorldMaterial material = WorldMaterial::None, CollisionCallback onCollisionEnter_ = nullptr,
             CollisionDir collisionDir = CollisionDir::ALL);
    Collider(Transform2D transform, Vector2i halflen, CollisionLayer::Layer layer, WorldMaterial material = WorldMaterial::None,
             CollisionCallback onCollisionEnter_ = nullptr, CollisionDir collisionDir = CollisionDir::ALL);

    // static creator functions
    static Collider Actor(AABB shape);
    static Collider Actor(Transform2D transform, Vector2i halflen);
    static Collider Solid(AABB shape, WorldMaterial material = WorldMaterial::None, CollisionCallback onCollisionEnter_ = nullptr,
                          CollisionDir collisionDir = CollisionDir::ALL);
    static Collider Solid(Transform2D transform, Vector2i halflen, WorldMaterial material = WorldMaterial::None,
                          CollisionCallback onCollisionEnter_ = nullptr, CollisionDir collisionDir = CollisionDir::ALL);
    static Collider SemiSolid(AABB shape, WorldMaterial material = WorldMaterial::None, CollisionCallback onCollisionEnter_ = nullptr,
                              CollisionDir collisionDir = CollisionDir::ALL);
    static Collider SemiSolid(Transform2D transform, Vector2i halflen, WorldMaterial material = WorldMaterial::None,
                              CollisionCallback onCollisionEnter_ = nullptr, CollisionDir collisionDir = CollisionDir::ALL);

    const AABB& getShape() const { return mShape; }
    AABB& getShapeMutable() { return mShape; }
    CollisionCallback getOnCollisionEnter() const { return mOnCollisionEnter; }
    void setCollisionCallback(CollisionCallback callback);
    WorldMaterial getMaterial() const { return mMaterial; }
    void setMaterial(WorldMaterial material) { mMaterial = material; }
    ecs::Entity getEntity() const { return mSelf; }
    void setEntity(ecs::Entity entity) { mSelf = entity; }
    bool isCollidable() const { return mIsCollidable; }
    void setIsCollidable(bool isCollidable) { mIsCollidable = isCollidable; }
    CollisionDir getCollisionDir() const { return mCollisionDir; }
    void setCollisionDir(CollisionDir dir) { mCollisionDir = dir; }
    CollisionLayer::Layer getCollisionLayer() const { return mCollisionLayer; }

    bool isActor() const { return mCollisionLayer & CollisionLayer::Actor; }
    bool isSolid() const { return mCollisionLayer & CollisionLayer::Solid; }
    bool isSemiSolid() const { return mCollisionLayer & CollisionLayer::SemiSolid; }
    bool isSolidAny() const { return LAYER_MATRIX.isSolidAny(mCollisionLayer); }

    bool move(const Vector2f amount, const CollisionCallback callback, bool isGroundedCheckNeeded = false, bool isManualMove = false,
              bool isPushedBySolid = false, bool updateRigidBodyFlags = false);
    HitInfo moveX(const Vector2f amountOriginal, const Vector2i amountRounded, const CollisionCallback callback);
    HitInfo moveY(const Vector2f amountOriginal, const Vector2i amountRounded, const CollisionCallback callback, bool isGroundedCheckNeeded = false);

    void moveNoCollisionCheck(Vector2f toMoveOriginal, Vector2i toMoveRounded);
    void pushAndCarry1D(Vector2f moveOriginal, Vector2i move1D, const std::vector<Collider*>& ridingColliders, bool isManualMove = false,
                        bool isPushedBySolid = false);
    bool emitCollisionInfo(const Vector2f amount, const HitInfo hitinfo, bool isXDirection, bool updateRigidBodyFlags);

    bool checkIsGrounded(const std::vector<Collider*>& otherColliders, Collider** dstGroundCollider);
    bool isGround() const;
    bool isRiding(const Collider* other) const;
    std::vector<Collider*> getRidingColliders() const;
    u16 getCollisionLayersThatCanStopMe() const;  // is this name specific enough?
    u16 getCollisionLayersThatCanRideMe() const;

    HitInfo checkCollision(const std::vector<Collider*>& colliders, const Vector2i position, const Vector2i moveNormal,
                           const u16 layerMask = CollisionLayer::ALL) const;
    void squish();

    // momentum:
    void setMomentum(const f32 momentum, const bool isXDirection);
    void maintainMomentum(const bool isXDirection);
    bool isMomentumStored() const { return mMomentumFramesLeft.x() > 0 || mMomentumFramesLeft.y() > 0; }
    void onMomentumNotUsed();
    void resetMomentum();
    Vector2f getMomentum() const { return mStoredMomentum; }

protected:
    bool tryCornerCorrection(const std::vector<Collider*>& others, Vector2i nextPos, s32 moveSignX, Vector2i moveNormal);
    void _pushAndCarry(s32 toMoveRounded, f32 toMoveUnrounded, bool isXDirection, s32 solidEdge, EdgeGetter edgeFunc,
                       const std::vector<Collider*>& riding, bool isManualMove, bool isPushedBySolid);

    // Shape mShape; // Maybe one day. Too much is hard coded to AABBs for me to bother rn
    AABB mShape;
    ecs::Entity mSelf;
    CollisionLayer::Layer mCollisionLayer;
    CollisionCallback mOnCollisionEnter;
    f32 mXRemainder = 0.0;
    f32 mYRemainder = 0.0;
    Vector2f mStoredMomentum = {0, 0};
    Vector2T<s16> mMomentumFramesLeft = {0, 0};  // so this class doesn't have padding
    WorldMaterial mMaterial;
    CollisionDir mCollisionDir;
    bool mIsCollidable = true;
    bool mIsAlive = true;
};

}  // namespace whal

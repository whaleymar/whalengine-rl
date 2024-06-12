#pragma once

#include <vector>

#include "Physics/CollisionLayer.h"
#include "Physics/IUseCollision.h"
#include "Physics/Material.h"
#include "Physics/Shapes.h"
#include "Util/Vector.h"

namespace whal {

class Collider;
struct HitInfo;
namespace ecs {
class Entity;
}

using CollisionCallbackNew = void (*)(ecs::Entity callbackEntity, ecs::Entity other, Collider* callbackEntityCollider, Collider* otherCollider,
                                      Vector2i hitNormal);

// TODO
// - momentum component
class Collider {
public:
    Collider() = default;
    Collider(AABB shape, CollisionLayer::Layer layer, WorldMaterial material = WorldMaterial::None, CollisionCallbackNew onCollisionEnter_ = nullptr,
             CollisionDir collisionDir = CollisionDir::ALL);
    Collider(Transform2D transform, Vector2i halflen, CollisionLayer::Layer layer, WorldMaterial material = WorldMaterial::None,
             CollisionCallbackNew onCollisionEnter_ = nullptr, CollisionDir collisionDir = CollisionDir::ALL);

    // static creator functions
    static Collider Actor(AABB shape);
    static Collider Actor(Transform2D transform, Vector2i halflen);
    static Collider Solid(AABB shape, WorldMaterial material = WorldMaterial::None, CollisionCallbackNew onCollisionEnter_ = nullptr,
                          CollisionDir collisionDir = CollisionDir::ALL);
    static Collider Solid(Transform2D transform, Vector2i halflen, WorldMaterial material = WorldMaterial::None,
                          CollisionCallbackNew onCollisionEnter_ = nullptr, CollisionDir collisionDir = CollisionDir::ALL);
    static Collider SemiSolid(AABB shape, WorldMaterial material = WorldMaterial::None, CollisionCallbackNew onCollisionEnter_ = nullptr,
                              CollisionDir collisionDir = CollisionDir::ALL);
    static Collider SemiSolid(Transform2D transform, Vector2i halflen, WorldMaterial material = WorldMaterial::None,
                              CollisionCallbackNew onCollisionEnter_ = nullptr, CollisionDir collisionDir = CollisionDir::ALL);

    const AABB& getCollider() const { return mShape; }  // TODO rename to getShape()
    AABB& getColliderMut() { return mShape; }           // TODO rename ^
    CollisionCallbackNew getOnCollisionEnter() const { return mOnCollisionEnter; }
    void setCollisionCallback(CollisionCallbackNew callback) { mOnCollisionEnter = callback; }  // was virtual
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

    void move(const Vector2f amount, const CollisionCallbackNew callback, bool isGroundedCheckNeeded = false, bool isManualMove = false);
    HitInfo moveX(const Vector2f amount, const CollisionCallbackNew callback);
    HitInfo moveY(const Vector2f amount, const CollisionCallbackNew callback, bool isGroundedCheckNeeded = false);

    // move as Solid (nothing can stop the collider)
    void moveNoCollisionCheck(f32 x, f32 y);
    void pushAndCarry(f32 x, f32 y, const std::vector<Collider*>& ridingColliders, bool isManualMove = false);

    bool checkIsGrounded(const std::vector<Collider*>& otherColliders, Collider** dstGroundCollider);
    bool isGround() const;
    bool isRiding(const Collider* other) const;
    std::vector<Collider*> getRidingColliders() const;
    u16 getCollisionLayersThatCanStopMe() const;  // is this name specific enough?

    HitInfo checkCollision(const std::vector<Collider*>& colliders, const Vector2i position, const Vector2i moveNormal,
                           const u16 layerMask = CollisionLayer::ALL) const;
    void squish();

protected:
    bool tryCornerCorrection(const std::vector<Collider*>& others, Vector2i nextPos, s32 moveSignX, Vector2i moveNormal);
    void _pushAndCarry(s32 toMoveRounded, f32 toMoveUnrounded, bool isXDirection, s32 solidEdge, EdgeGetter edgeFunc,
                       const std::vector<Collider*>& riding, bool isManualMove) const;

    // Shape mShape; // Maybe one day. Too much is hard coded to AABBs for me to bother rn
    AABB mShape;
    ecs::Entity mSelf;
    CollisionLayer::Layer mCollisionLayer;
    CollisionCallbackNew mOnCollisionEnter;
    f32 mXRemainder = 0.0;
    f32 mYRemainder = 0.0;
    WorldMaterial mMaterial;
    CollisionDir mCollisionDir;
    bool mIsCollidable = true;
};

}  // namespace whal

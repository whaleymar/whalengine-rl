#pragma once

#include "Physics/Collision/Shapes.h"
#include "Physics/Material.h"
#include "whalECS/src/ECS.h"

namespace whal {

class IUseCollision;

using CollisionCallback = void (*)(ecs::Entity callbackEntity, ecs::Entity other, IUseCollision* callbackEntityCollider, IUseCollision* otherCollider,
                                   Vector2i hitNormal);

class IUseCollision {
public:
    IUseCollision() = default;
    IUseCollision(AABB2 collider, WorldMaterial material = WorldMaterial::None, CollisionCallback callback = nullptr);
    const AABB2& getCollider() const { return mCollider; }
    AABB2& getColliderMut() { return mCollider; }
    CollisionCallback getOnCollisionEnter() const { return mOnCollisionEnter; }
    virtual void setCollisionCallback(CollisionCallback callback);
    WorldMaterial getMaterial() const { return mMaterial; }
    void setMaterial(WorldMaterial material) { mMaterial = material; }
    ecs::Entity getEntity() const { return mSelf; }
    bool isCollidable() const { return mIsCollidable; }
    void setIsCollidable(bool isCollidable) { mIsCollidable = isCollidable; }
    void setEntity(ecs::Entity entity) { mSelf = entity; }

    virtual void squish() { getEntity().kill(); }

protected:
    AABB2 mCollider;
    ecs::Entity mSelf;
    f32 mXRemainder = 0.0;
    f32 mYRemainder = 0.0;
    CollisionCallback mOnCollisionEnter;
    WorldMaterial mMaterial;
    bool mIsCollidable = true;
};

enum class CollisionDir : u8 { ALL, LEFT, RIGHT, DOWN, UP };

// returns true if a collision CAN happen given the move normal & collision direction
bool checkDirectionalCollision(const AABB2& actor, const AABB2& solid, Vector2i moveNormal, CollisionDir collisionDir);

}  // namespace whal

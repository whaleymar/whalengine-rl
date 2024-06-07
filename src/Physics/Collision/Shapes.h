#pragma once

#include "Physics/Collision/HitInfo.h"
#include "Physics/Collision/ICollider.h"

namespace whal {

class AABB2 : public IColliderShape {
public:
    AABB2(Vector2i center = {0, 0}, Vector2i half = {0, 0}, CollisionLayer::Layer layer = CollisionLayer::None);
    AABB2(Transform2D transform, Vector2i half, CollisionLayer::Layer layer);

    using IColliderShape::setPosition;
    void setPosition(Transform2D transform);
    bool isOverlapping(const IColliderShape* other) const override;
    // std::unique_ptr<IColliderShape> clone() const override;
#ifndef NDEBUG
    void draw(Vector2f cameraPos, Color color) const override;
#endif

    Vector2i getHalf() const { return mHalf; }
    void setHalf(Vector2i half) { mHalf = half; }
    Vector2i getPositionEdge(Vector2i unitDir) const;
    const std::optional<HitInfo> collide(const AABB2& other) const;

    s32 top() const { return mCenter.y() + mHalf.y(); }
    s32 bottom() const { return mCenter.y() - mHalf.y(); }
    s32 right() const { return mCenter.x() + mHalf.x(); }
    s32 left() const { return mCenter.x() - mHalf.x(); }

private:
    Vector2i mHalf;
};

using EdgeGetter = s32 (AABB2::*)() const;

class Circle2 : public IColliderShape {
public:
    Circle2(Vector2i center = {0, 0}, s32 radius = 0, CollisionLayer::Layer layer = CollisionLayer::None);
    Circle2(Transform2D transform, s32 radius, CollisionLayer::Layer layer);

    using IColliderShape::setPosition;
    void setPosition(Transform2D transform);

    bool isOverlapping(const IColliderShape* other) const override;
    // std::unique_ptr<IColliderShape> clone() const override;
#ifndef NDEBUG
    void draw(Vector2f cameraPos, Color color) const override;
#endif

    s32 getRadius() const { return mRadius; }
    f32 getDistanceFromCenter(const AABB2* aabb) const;
    f32 getDistanceFromCenter(const Circle2* other) const;

private:
    s32 mRadius;
};

bool isIntersectAABBvsAABB(const AABB2*, const AABB2*);
bool isIntersectCirclevsCircle(const Circle2*, const Circle2*);
bool isIntersectAABBvsCircle(const AABB2*, const Circle2*);

}  // namespace whal

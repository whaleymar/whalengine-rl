#pragma once

#include "Util/Vector.h"

typedef struct Color Color;

namespace whal {

struct HitInfo;
struct Transform2D;

class AABB {
public:
    AABB() = default;
    AABB(Vector2i center, Vector2i half = {0, 0});
    AABB(Transform2D transform, Vector2i half, Vector2i colliderOffset);

    static AABB fromPoints(Vector2i p1, Vector2i p2);
    static AABB fromPoints(Vector2i p1, Vector2i p2, Vector2i p3);

    void setPosition(Vector2i center);
    void setPosition(Transform2D transform, Vector2i colliderOffset);
    Vector2i getPosition() const { return mCenter; }

    Vector2i getHalf() const { return mHalf; }
    void setHalf(Vector2i half) { mHalf = half; }
    Vector2i getPositionEdge(Vector2i unitDir) const;
    Vector2i getClosestPointTo(Vector2i point) const;
    HitInfo collide(const AABB& other) const;

    // NOT used by Shape, but is used by Collider
    bool isOverlapping(const AABB& other) const;
    bool contains(const AABB& other) const;
    bool contains(Vector2i point) const;
#ifndef NDEBUG
    void draw(Vector2f cameraPos, Color color) const;
#endif

    s32 top() const { return mCenter.y + mHalf.y; }
    s32 bottom() const { return mCenter.y - mHalf.y; }
    s32 right() const { return mCenter.x + mHalf.x; }
    s32 left() const { return mCenter.x - mHalf.x; }

private:
    Vector2i mCenter;
    Vector2i mHalf;
};

using EdgeGetter = s32 (AABB::*)() const;

class Circle {
public:
    Circle() = default;
    Circle(Vector2i center, s32 radius = 0);
    Circle(Transform2D transform, s32 radius);

    void setPosition(Vector2i center);
    void setPosition(Transform2D transform, Vector2i colliderOffset);
    Vector2i getPosition() const { return mCenter; }

    s32 getRadius() const { return mRadius; }
    f32 getDistanceFromCenter(const AABB& aabb) const;
    f32 getDistanceFromCenter(const Circle& other) const;
    Vector2f getVecToClosestPoint(const AABB& aabb) const;

    AABB getBoundingBox() const;

#ifndef NDEBUG
    void draw(Vector2f cameraPos, Color color) const;
#endif

private:
    Vector2i mCenter;
    s32 mRadius;
};

enum class ShapeTag : u16 { AABB, Circle };

// tagged union
class Shape {
public:
    Shape() : mAABB(Vector2i(5, 5)), mShape(ShapeTag::AABB) {}

    Shape(AABB aabb);
    Shape(Circle circle);

    // apparently these get deleted bc compiler bug
    Shape(const Shape& other);
    Shape& operator=(const Shape& other);

    ShapeTag getShape() const { return mShape; }

    AABB getAABB() const;
    Circle getCircle() const;
    AABB getBoundingBox() const;

    void setPosition(Vector2i center);
    void setPosition(Transform2D transform, Vector2i colliderOffset);
    Vector2i getPosition() const;
    bool isOverlapping(const Shape& other) const;
    bool isOverlapping(const AABB& other) const;
    bool isOverlapping(const Circle& other) const;
#ifndef NDEBUG
    void draw(Vector2f cameraPos, Color color) const;
#endif

private:
    union {
        AABB mAABB;
        Circle mCircle;
    };
    ShapeTag mShape;
};

bool isIntersectAABBvsAABB(const AABB&, const AABB&);
bool isIntersectCirclevsCircle(const Circle&, const Circle&);
bool isIntersectAABBvsCircle(const AABB&, const Circle&);

}  // namespace whal

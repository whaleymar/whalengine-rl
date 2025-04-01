#pragma once

#include "Util/Vector.h"

#ifndef NDEBUG
namespace rl {
typedef struct Color Color;
}
#endif

namespace whal {

struct HitInfo;
struct Transform;
struct Color;

class AABB {
public:
    AABB() = default;
    AABB(Vector2i center, Vector2i half = {0, 0});
    AABB(Transform transform, Vector2i half, Vector2i relativeOffset = Vector2i::ZERO);

    static AABB fromPoints(Vector2i p1, Vector2i p2);
    static AABB fromPoints(Vector2i p1, Vector2i p2, Vector2i p3);

    void setPosition(Vector2i center);
    void setPosition(Transform transform, Vector2i relativeOffset);
    Vector2i getPosition() const { return mCenter; }
    Vector2i& getPositionMut() { return mCenter; }

    Vector2i getHalf() const { return mHalf; }
    Vector2i& getHalfMut() { return mHalf; }
    void setHalf(Vector2i half) { mHalf = half; }
    Vector2i getPositionEdge(Vector2i unitDir) const;
    Vector2i getClosestPointTo(Vector2i point) const;
    HitInfo collide(const AABB& other) const;

    // NOT used by Shape, but is used by Collider and Renderer.
    // Inlining to speed up renderer.
    inline bool isOverlapping(const AABB& other) const {
        // const auto delta = other.mCenter - mCenter;
        // const auto overlap = mHalf + other.mHalf;
        // return overlap.x > abs(delta.x) && overlap.y > abs(delta.y);
        return (mHalf.x + other.mHalf.x) > math::abs(other.mCenter.x - mCenter.x) &&
               (mHalf.y + other.mHalf.y) > math::abs(other.mCenter.y - mCenter.y);
    }

    bool contains(const AABB& other) const;
    bool contains(Vector2i point) const;
#ifndef NDEBUG
    void draw(Color color) const;
#endif

    s32 top() const { return mCenter.y + mHalf.y; }
    s32 bottom() const { return mCenter.y - mHalf.y; }
    s32 right() const { return mCenter.x + mHalf.x; }
    s32 left() const { return mCenter.x - mHalf.x; }

private:
    Vector2i mCenter;
    Vector2i mHalf;

public:
    struct AABBDisplay {
        Vector2i _center;
        Vector2i half;
    };

    using ReflectionType = AABBDisplay;
    AABB(AABBDisplay display) : mCenter(display._center), mHalf(display.half) {}
    ReflectionType reflection() const {
        return AABBDisplay{
            ._center = mCenter,
            .half = mHalf,
        };
    }
};

using EdgeGetter = s32 (AABB::*)() const;

class Circle {
public:
    Circle() = default;
    Circle(Vector2i center, s32 radius = 0);
    Circle(Transform transform, s32 radius, Vector2i relativeOffset = Vector2i::ZERO);

    void setPosition(Vector2i center);
    void setPosition(Transform transform, Vector2i relativeOffset);
    Vector2i getPosition() const { return mCenter; }

    s32 getRadius() const { return mRadius; }
    f32 getDistanceFromCenter(const AABB& aabb) const;
    f32 getDistanceFromCenter(const Circle& other) const;
    Vector2f getVecToClosestPoint(const AABB& aabb) const;

    AABB getBoundingBox() const;

#ifndef NDEBUG
    void draw(Color color) const;
#endif

private:
    Vector2i mCenter;
    s32 mRadius;

public:
    struct CircleDisplay {
        Vector2i _center;
        s32 radius;
    };

    using ReflectionType = CircleDisplay;
    Circle(CircleDisplay display) : mCenter(display._center), mRadius(display.radius) {}
    ReflectionType reflection() const {
        return CircleDisplay{
            ._center = mCenter,
            .radius = mRadius,
        };
    }
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
    AABB& getAABBMut();
    Circle getCircle() const;
    Circle& getCircleMut();
    AABB getBoundingBox() const;

    void setPosition(Vector2i center);
    void setPosition(Transform transform, Vector2i colliderOffset);
    Vector2i getPosition() const;
    bool isOverlapping(const Shape& other) const;
    bool isOverlapping(const AABB& other) const;
    bool isOverlapping(const Circle& other) const;
#ifndef NDEBUG
    void draw(Color color) const;
#endif

private:
    union {
        AABB mAABB;
        Circle mCircle;
    };
    ShapeTag mShape;

public:
    struct ShapeDisplay {
        ShapeTag tag;
        Vector2i _center;
        Vector2i size;
    };

    using ReflectionType = ShapeDisplay;

    Shape(ShapeDisplay display) {
        mShape = display.tag;
        switch (display.tag) {
        case ShapeTag::AABB:
            mAABB = AABB(display._center, display.size);
            break;
        case ShapeTag::Circle:
            mCircle = Circle(display._center, display.size.x);
            break;
        }
    }

    ShapeDisplay reflection() const {
        if (mShape == ShapeTag::AABB) {
            return ShapeDisplay{
                .tag = mShape,
                ._center = mAABB.getPosition(),
                .size = mAABB.getHalf(),
            };
        } else {
            return ShapeDisplay{
                .tag = mShape,
                ._center = mCircle.getPosition(),
                .size = {mCircle.getRadius(), mCircle.getRadius()},
            };
        }
    }
};

bool isIntersectAABBvsAABB(const AABB&, const AABB&);
bool isIntersectCirclevsCircle(const Circle&, const Circle&);
bool isIntersectAABBvsCircle(const AABB&, const Circle&);

}  // namespace whal

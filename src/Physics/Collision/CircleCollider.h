#pragma once

#include "Util/Vector.h"

namespace whal {

struct Transform2D;
struct AABB;

struct Circle {
    Vector2i center;
    s32 radius;

    Circle() = default;
    Circle(Transform2D transform, s32 radius);

    void setPosition(Vector2i position);
    void setPositionFromBottom(Vector2i position);
    f32 distanceFrom(const Circle& other) const;
    f32 distanceFrom(const AABB& other) const;
    bool isOverlapping(const Circle& other) const;
    bool isOverlapping(const AABB& other) const;
};

}  // namespace whal

#pragma once

namespace whal {

class AABB;

class Box {
public:
    Box() = default;
    Box(Vector2i center, Vector2i half, f32 rotationDegrees);
    Box(AABB aabb, f32 rotationDegrees);

    AABB getBoundingAABB() const;

private:
    Vector2i mCenter;
    Vector2i mHalf;
    f32 mRotationDegrees;
};

}  // namespace whal

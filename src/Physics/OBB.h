#pragma once

#include "Util/Vector.h"

namespace whal {

class AABB;

// Oriented Bounding Box
class OBB {
public:
    OBB() = default;
    OBB(Vector2i center, Vector2i half, f32 rotationDegrees);
    OBB(AABB aabb, f32 rotationDegrees);

    AABB getBoundingAABB() const;

private:
    Vector2i mCenter;
    Vector2i mHalf;
    f32 mRotationDegrees;
};

}  // namespace whal

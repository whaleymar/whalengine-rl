#include "OBB.h"

#include <algorithm>
#include <cmath>

#include "Shapes.h"

namespace whal {

OBB::OBB(Vector2i center, Vector2i half, f32 rotationDegrees) : mCenter(center), mHalf(half), mRotationDegrees(rotationDegrees) {}

OBB::OBB(AABB aabb, f32 rotationDegrees) : mCenter(aabb.getPosition()), mHalf(aabb.getHalf()), mRotationDegrees(rotationDegrees) {}

AABB OBB::getBoundingAABB() const {
    // optimize for common case
    if (mRotationDegrees == 0.0f) {
        return AABB(mCenter, mHalf);
    }

    Vector2f p1 = (mCenter + mHalf).as<f32>();
    Vector2f p2 = (mCenter + mHalf * Vector2i(-1, 1)).as<f32>();
    Vector2f p3 = (mCenter + mHalf * Vector2i(1, -1)).as<f32>();
    Vector2f p4 = (mCenter - mHalf).as<f32>();

    const Vector2f centerf = mCenter.as<f32>();
    p1 = p1.rotate(mRotationDegrees, centerf);
    p2 = p2.rotate(mRotationDegrees, centerf);
    p3 = p3.rotate(mRotationDegrees, centerf);
    p4 = p4.rotate(mRotationDegrees, centerf);

    const f32 minX = std::min({p1.x, p2.x, p3.x, p4.x});
    const f32 maxX = std::max({p1.x, p2.x, p3.x, p4.x});
    const f32 minY = std::min({p1.y, p2.y, p3.y, p4.y});
    const f32 maxY = std::max({p1.y, p2.y, p3.y, p4.y});

    const Vector2i newHalf(std::ceil((maxX - minX) / 2.0f), std::ceil((maxY - minY) / 2.0f));
    return AABB(mCenter, newHalf);
}

}  // namespace whal

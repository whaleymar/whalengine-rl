#include "CircleCollider.h"

#include "ECS/Transform.h"
#include "Physics/Collision/AABB.h"
#include "Util/MathUtil.h"
#include "Util/Vector.h"

namespace whal {

Circle::Circle(Transform2D transform, s32 radius_) : center(transform.position.x(), transform.position.y() + radius_), radius(radius_) {}

void Circle::setPosition(Vector2i position) {
    center = position;
}

void Circle::setPositionFromBottom(Vector2i position) {
    center = {position.x(), position.y() + radius};
}

f32 Circle::distanceFrom(const Circle& other) const {
    return toFloatVec(center - other.center).len();
}

f32 Circle::distanceFrom(const AABB& other) const {
    const auto delta = center - other.center;  // vector from AABB's center to circle's

    // clamp to be on the AABB's boundary. Is now the point on the AABB closest to the circle
    const auto closestPoint =
        other.center + Vector2i(clamp(delta.x(), -other.half.x(), other.half.x()), clamp(delta.y(), -other.half.y(), other.half.y()));
    return toFloatVec(center - closestPoint).len();
}

bool Circle::isOverlapping(const Circle& other) const {
    return distanceFrom(other) <= static_cast<f32>(std::max(radius, other.radius));
}

bool Circle::isOverlapping(const AABB& other) const {
    return distanceFrom(other) <= radius;
}

}  // namespace whal

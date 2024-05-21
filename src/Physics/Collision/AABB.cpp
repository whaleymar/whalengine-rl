#include "Physics/Collision/AABB.h"

#include "ECS/Components/Transform.h"
#include "Util/MathUtil.h"
#include "Util/Vector.h"

namespace whal {

AABB::AABB(Vector2i half_) : center(Vector2i::zero), half(half_) {}
AABB::AABB(Vector2i center_, Vector2i half_) : center(center_), half(half_) {}

AABB::AABB(Transform transform, Vector2i half_) : center({transform.position.x(), transform.position.y() + half_.y()}), half(half_) {}

void AABB::setPosition(Vector2i position) {
    center = position;
}

void AABB::setPosition(Vector2f position) {
    center = Vector2i(static_cast<s32>(position.x()), static_cast<s32>(position.y()));
}

void AABB::setPositionFromBottom(Vector2i bottom) {
    center = {bottom.x(), bottom.y() + half.y()};
}

void AABB::setHalflen(Vector2i half_) {
    half = half_;
}

const std::optional<HitInfo> AABB::collide(const AABB& other) const {
    const auto delta = other.center - center;
    const auto overlap = half + other.half;

    const s32 px = overlap.x() - abs(delta.x());
    if (px <= 0) {
        return std::nullopt;
    }

    const s32 py = overlap.y() - abs(delta.y());
    if (py <= 0) {
        return std::nullopt;
    }

    if (px == py) {
        const s32 signX = sign(delta.x());
        const s32 signY = sign(delta.y());
        // Vector2i hitPos(center.x() + half.x() * signX, center.y() + half.y() * signY);
        // Vector2i hitDelta(px * signX, py * signY);
        Vector2i hitNormal(signX, signY);
        return HitInfo(hitNormal);
    }
    if (px < py) {
        const s32 signX = sign(delta.x());
        // Vector2i hitPos(center.x() + half.x() * signX, other.center.y());
        // Vector2i hitDelta(px * signX, 0);
        Vector2i hitNormal(signX, 0);
        return HitInfo(hitNormal);
    } else {
        const s32 signY = sign(delta.y());
        // Vector2i hitPos(other.center.x(), center.y() + half.y() * signY);
        // Vector2i hitDelta(0, py * signY);
        Vector2i hitNormal(0, signY);
        return HitInfo(hitNormal);
    }
}

const std::optional<HitInfo> AABB::collide(Segment segment) const {
    const s32 scaleX = 1 / segment.delta.x();
    const s32 scaleY = 1 / segment.delta.y();
    const s32 signX = sign(segment.delta.x());
    const s32 signY = sign(segment.delta.y());

    const s32 nearX = (center.x() - signX * half.x() - segment.origin.x()) * scaleX;
    const s32 nearY = (center.y() - signY * half.y() - segment.origin.y()) * scaleY;

    const s32 farX = (center.x() + signX * half.x() - segment.origin.x()) * scaleX;
    const s32 farY = (center.y() + signY * half.y() - segment.origin.y()) * scaleY;

    if (nearX > farY || nearY > farX) {
        // segment missed the AABB
        return std::nullopt;
    }

    const s32 near = nearX > nearY ? nearX : nearY;
    const s32 far = farX < farY ? farX : farY;
    if (near >= 1 || far <= 0) {
        // segment ends before reaching nearest edge
        // OR segment points away from AABB
        return std::nullopt;
    }

    // collision is happening
    // "If the near time is greater than zero, the segment starts outside and is entering the box. Otherwise, the segment starts inside the box, and
    // is exiting it. If we’re entering the box, we can set the hit time to the near time, since that’s the point along the segment at which it
    // collided. If it’s inside, it’s colliding at the very starting of the line, so just set the hit time to zero."
    Vector2i hitNormal = Vector2i::zero;
    if (nearX == nearY) {
        hitNormal.e[0] = -signX;
        hitNormal.e[1] = -signY;
    } else if (nearX > nearY) {
        hitNormal.e[0] = -signX;
    } else {
        hitNormal.e[1] = -signY;
    }
    // const s32 uncollidedFraction = clamp(near, 0, 1);
    // Vector2i hitDelta = segment.delta * (-1 * (1 - uncollidedFraction));
    // Vector2i hitPos = segment.delta * uncollidedFraction + segment.origin;
    return HitInfo(hitNormal);
}

bool AABB::isOverlapping(const AABB& other) const {
    const auto delta = other.center - center;
    const auto overlap = half + other.half;
    return overlap.x() > abs(delta.x()) && overlap.y() > abs(delta.y());
}

Vector2i AABB::getPositionEdge(Vector2i unitDir) const {
    return center + half * unitDir;
}

}  // namespace whal

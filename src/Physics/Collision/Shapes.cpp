#include "Shapes.h"

#include <raylib.h>

#include "ECS/Transform.h"

namespace whal {

AABB2::AABB2(Vector2i center, Vector2i half, CollisionLayer::Layer layer) : IColliderShape(center, ColliderShape::AABB, layer), mHalf(half) {}

AABB2::AABB2(Transform2D transform, Vector2i half, CollisionLayer::Layer layer)
    : IColliderShape({transform.position.x(), transform.position.y() + half.y()}, ColliderShape::AABB, layer), mHalf(half) {}

void AABB2::setPosition(Transform2D transform) {
    mCenter = Vector2i(transform.position.x(), transform.position.y() + mHalf.y());
}

bool AABB2::isOverlapping(const IColliderShape* other) const {
    switch (other->getShape()) {
    case ColliderShape::AABB:
        return isIntersectAABBvsAABB(this, static_cast<const AABB2*>(other));
    case ColliderShape::Circle:
        return isIntersectAABBvsCircle(this, static_cast<const Circle2*>(other));
    }
}

// std::unique_ptr<IColliderShape> AABB2::clone() const {
//     AABB2 other(*this);
//     return std::make_unique<AABB2>(other);
// }

#ifndef NDEBUG
void AABB2::draw(Vector2f cameraPos, Color color) const {
    Vector2f position(left(), bottom());
    Vector2f size = Vector2f(mHalf.x(), mHalf.y()) * 2;

    // subtract size.y() so we draw from bottom left instead of top left
    Vector2f dstPosition = {position.x() - cameraPos.x(), -1 * position.y() + cameraPos.y() - size.y()};
    DrawRectangleLines(dstPosition.x(), dstPosition.y(), size.x(), size.y(), color);
}
#endif

const std::optional<HitInfo> AABB2::collide(const AABB2& other) const {
    const auto delta = other.mCenter - mCenter;
    const auto overlap = mHalf + other.mHalf;

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

Vector2i AABB2::getPositionEdge(Vector2i unitDir) const {
    return mCenter + mHalf * unitDir;
}

Circle2::Circle2(Vector2i center, s32 radius, CollisionLayer::Layer layer) : IColliderShape(center, ColliderShape::Circle, layer), mRadius(radius) {}

Circle2::Circle2(Transform2D transform, s32 radius, CollisionLayer::Layer layer)
    : IColliderShape({transform.position.x(), transform.position.y() + radius}, ColliderShape::Circle, layer), mRadius(radius) {}

void Circle2::setPosition(Transform2D transform) {
    mCenter = Vector2i(transform.position.x(), transform.position.y() + mRadius);
}

bool Circle2::isOverlapping(const IColliderShape* other) const {
    switch (other->getShape()) {
    case ColliderShape::AABB:
        return isIntersectAABBvsCircle(static_cast<const AABB2*>(other), this);
    case ColliderShape::Circle:
        return isIntersectCirclevsCircle(this, static_cast<const Circle2*>(other));
    }
}

// std::unique_ptr<IColliderShape> Circle2::clone() const {
//     Circle2 other(*this);
//     return std::make_unique<Circle2>(other);
// }

#ifndef NDEBUG
void Circle2::draw(Vector2f cameraPos, Color color) const {
    Vector2f dstPosition = {mCenter.x() - cameraPos.x(), -1 * mCenter.y() + cameraPos.y()};
    DrawCircleLines(dstPosition.x(), dstPosition.y(), mRadius, color);
}
#endif

f32 Circle2::getDistanceFromCenter(const AABB2* aabb) const {
    const auto delta = getPosition() - aabb->getPosition();  // vector from AABB's center to circle's

    // clamp to be on the AABB's boundary. Is now the point on the AABB closest to the circle
    const auto half = aabb->getHalf();
    const auto closestPoint = aabb->getPosition() + Vector2i(clamp(delta.x(), -half.x(), half.x()), clamp(delta.y(), -half.y(), half.y()));
    return toFloatVec(getPosition() - closestPoint).len();
}

f32 Circle2::getDistanceFromCenter(const Circle2* other) const {
    return toFloatVec(getPosition() - other->getPosition()).len() - other->getRadius();
}

bool isIntersectAABBvsAABB(const AABB2* first, const AABB2* other) {
    const auto delta = other->getPosition() - first->getPosition();
    const auto overlap = first->getHalf() + other->getHalf();
    return overlap.x() > abs(delta.x()) && overlap.y() > abs(delta.y());
}

bool isIntersectCirclevsCircle(const Circle2* first, const Circle2* other) {
    return first->getDistanceFromCenter(other) <= first->getRadius();
}

bool isIntersectAABBvsCircle(const AABB2* aabb, const Circle2* circle) {
    return circle->getDistanceFromCenter(aabb) <= circle->getRadius();
}

}  // namespace whal

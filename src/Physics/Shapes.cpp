#include "Shapes.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <raylib.h>

#include "Components/Transform.h"
#include "Physics/HitInfo.h"
#include "Util/MathUtil.h"

#ifndef NDEBUG
#include "Gfx/Color.h"
#include "Settings.h"
#endif

namespace whal {

AABB::AABB(Vector2i center, Vector2i half) : mCenter(center), mHalf(half) {}

// note: mHalf is not scaled with the transform, but mCenter's location does. mHalf should eventually scale, but it requires some effort
AABB::AABB(Transform transform, Vector2i half, Vector2i offset) : mCenter(transform.apply2D(offset)), mHalf(half) {}

AABB AABB::fromPoints(Vector2i p1, Vector2i p2) {
    Vector2i min;
    Vector2i max;
    if (p1.x < p2.x) {
        min.x = p1.x;
        max.x = p2.x;
    } else {
        min.x = p2.x;
        max.x = p1.x;
    }

    if (p1.y < p2.y) {
        min.y = p1.y;
        max.y = p2.y;
    } else {
        min.y = p2.y;
        max.y = p1.y;
    }

    Vector2i half = ((max - min).as<f32>() * 0.5).round();
    return AABB(min + half, half);
}

AABB AABB::fromPoints(Vector2i p1, Vector2i p2, Vector2i p3) {
    s32 minX = std::min({p1.x, p2.x, p3.x});
    s32 maxX = std::max({p1.x, p2.x, p3.x});
    s32 minY = std::min({p1.y, p2.y, p3.y});
    s32 maxY = std::max({p1.y, p2.y, p3.y});
    Vector2i minV(minX, minY);
    Vector2i maxV(maxX, maxY);

    Vector2i half = ((maxV - minV).as<f32>() * 0.5).round();
    return AABB(minV + half, half);
}

void AABB::setPosition(Vector2i center) {
    mCenter = center;
}

void AABB::setPosition(Transform transform, Vector2i relativeOffset) {
    mCenter = transform.apply2D(relativeOffset);
}

bool AABB::contains(const AABB& other) const {
    return left() <= other.left() && other.right() <= right() && other.top() <= top() && bottom() <= other.bottom();
}

bool AABB::contains(Vector2i point) const {
    return left() <= point.x && point.x <= right() && point.y <= top() && bottom() <= point.y;
}

#ifndef NDEBUG

void AABB::draw(Color color) const {
    Vector2f position(left(), bottom());
    Vector2f size = Vector2f(mHalf.x, mHalf.y) * 2;

    // subtract size.y so we draw from bottom left instead of top left
    Vector2f dstPosition = {position.x, -position.y - size.y};

    dstPosition *= VIRTUAL_SCREEN_RATIO;
    size *= VIRTUAL_SCREEN_RATIO;
    f32 thickness = std::max(1.0f, 0.5f * VIRTUAL_SCREEN_RATIO);

    rl::DrawRectangleLinesEx(rl::Rectangle(dstPosition.x, dstPosition.y, size.x, size.y), thickness, color.asLDR());
}
#endif

HitInfo AABB::collide(const AABB& other) const {
    const auto delta = other.mCenter - mCenter;
    const auto overlap = mHalf + other.mHalf;

    const s32 px = overlap.x - abs(delta.x);
    if (px <= 0) {
        return HitInfo();
    }

    const s32 py = overlap.y - abs(delta.y);
    if (py <= 0) {
        return HitInfo();
    }

    if (px == py) {
        const s32 signX = math::sign(delta.x);
        const s32 signY = math::sign(delta.y);
        // Vector2i hitPos(center.x + half.x * signX, center.y + half.y * signY);
        // Vector2i hitDelta(px * signX, py * signY);
        Vector2i hitNormal(signX, signY);
        return HitInfo(hitNormal, true);
    }
    if (px < py) {
        const s32 signX = math::sign(delta.x);
        // Vector2i hitPos(center.x + half.x * signX, other.center.y);
        // Vector2i hitDelta(px * signX, 0);
        Vector2i hitNormal(signX, 0);
        return HitInfo(hitNormal, true);
    } else {
        const s32 signY = math::sign(delta.y);
        // Vector2i hitPos(other.center.x, center.y + half.y * signY);
        // Vector2i hitDelta(0, py * signY);
        Vector2i hitNormal(0, signY);
        return HitInfo(hitNormal, true);
    }
}

Vector2i AABB::getPositionEdge(Vector2i unitDir) const {
    return mCenter + mHalf * unitDir;
}

Vector2i AABB::getClosestPointTo(Vector2i point) const {
    const auto delta = point - mCenter;  // vector from center to point

    // clamp to be on the AABB's boundary
    return mCenter + Vector2i(math::clamp(delta.x, -mHalf.x, mHalf.x), math::clamp(delta.y, -mHalf.y, mHalf.y));
}

Circle::Circle(Vector2i center, s32 radius) : mCenter(center), mRadius(radius) {}

Circle::Circle(Transform transform, s32 radius, Vector2i offset) : mCenter(transform.apply2D(offset)), mRadius(radius) {}

void Circle::setPosition(Vector2i center) {
    mCenter = center;
}

void Circle::setPosition(Transform transform, Vector2i offset) {
    mCenter = transform.apply2D(offset);
}

#ifndef NDEBUG
void Circle::draw(Color color) const {
    Vector2f dstPosition(mCenter.x, -mCenter.y);
    dstPosition *= VIRTUAL_SCREEN_RATIO;
    rl::DrawCircleLines(dstPosition.x, dstPosition.y, mRadius * VIRTUAL_SCREEN_RATIO, color.asLDR());  // no thickness param :(
}
#endif

f32 Circle::getDistanceFromCenter(const AABB& aabb) const {
    const auto closestPoint = aabb.getClosestPointTo(getPosition());
    return (getPosition() - closestPoint).as<f32>().len();
}

f32 Circle::getDistanceFromCenter(const Circle& other) const {
    return (getPosition() - other.getPosition()).as<f32>().len() - other.getRadius();
}

// calculates a vector from the circle's origin to the closest point on the given AABB
Vector2f Circle::getVecToClosestPoint(const AABB& aabb) const {
    const auto delta = getPosition() - aabb.getPosition();  // vector from AABB's center to circle's

    // clamp to be on the AABB's boundary. Is now the point on the AABB closest to the circle
    const auto half = aabb.getHalf();
    const auto closestPoint = aabb.getPosition() + Vector2i(math::clamp(delta.x, -half.x, half.x), math::clamp(delta.y, -half.y, half.y));
    return (closestPoint - getPosition()).as<f32>();
}

AABB Circle::getBoundingBox() const {
    return AABB(mCenter, {mRadius, mRadius});
}

bool isIntersectAABBvsAABB(const AABB& first, const AABB& other) {
    const auto delta = other.getPosition() - first.getPosition();
    const auto overlap = first.getHalf() + other.getHalf();
    return overlap.x > abs(delta.x) && overlap.y > abs(delta.y);
}

bool isIntersectCirclevsCircle(const Circle& first, const Circle& other) {
    return first.getDistanceFromCenter(other) <= first.getRadius();
}

bool isIntersectAABBvsCircle(const AABB& aabb, const Circle& circle) {
    return circle.getDistanceFromCenter(aabb) <= circle.getRadius();
}

Shape::Shape(AABB aabb) : mAABB(aabb), mShape(ShapeTag::AABB) {}

Shape::Shape(Circle circle) : mCircle(circle), mShape(ShapeTag::Circle) {}

Shape::Shape(const Shape& other) {
    std::memcpy((void*)this, (void*)&other, sizeof(other));
}

Shape& Shape::operator=(const Shape& other) {
    if (this == &other) {
        return *this;
    }
    std::memcpy((void*)this, (void*)&other, sizeof(other));
    return *this;
}

Circle Shape::getCircle() const {
    assert(mShape == ShapeTag::Circle && "trying to run getCircle but ColliderShape is not a circle");
    return mCircle;
}

AABB Shape::getBoundingBox() const {
    switch (mShape) {
    case ShapeTag::AABB:
        return mAABB;
    case ShapeTag::Circle:
        return mCircle.getBoundingBox();
    }
}

AABB Shape::getAABB() const {
    assert(mShape == ShapeTag::AABB && "trying to run getAABB but ColliderShape is not an AABB");
    return mAABB;
}

void Shape::setPosition(Vector2i center) {
    switch (mShape) {
    case ShapeTag::AABB:
        mAABB.setPosition(center);
        break;
    case ShapeTag::Circle:
        mCircle.setPosition(center);
        break;
    }
}

void Shape::setPosition(Transform transform, Vector2i colliderOffset) {
    switch (mShape) {
    case ShapeTag::AABB:
        mAABB.setPosition(transform, colliderOffset);
        break;
    case ShapeTag::Circle:
        mCircle.setPosition(transform, colliderOffset);
        break;
    }
}

Vector2i Shape::getPosition() const {
    switch (mShape) {
    case ShapeTag::AABB:
        return mAABB.getPosition();
    case ShapeTag::Circle:
        return mCircle.getPosition();
    }
}

bool Shape::isOverlapping(const Shape& other) const {
    switch (other.mShape) {
    case ShapeTag::AABB:
        return isOverlapping(other.mAABB);
    case ShapeTag::Circle:
        return isOverlapping(other.mCircle);
    }
}

bool Shape::isOverlapping(const AABB& other) const {
    switch (mShape) {
    case ShapeTag::AABB:
        return isIntersectAABBvsAABB(mAABB, other);
    case ShapeTag::Circle:
        return isIntersectAABBvsCircle(other, mCircle);
    }
}

bool Shape::isOverlapping(const Circle& other) const {
    switch (mShape) {
    case ShapeTag::AABB:
        return isIntersectAABBvsCircle(mAABB, other);
    case ShapeTag::Circle:
        return isIntersectCirclevsCircle(other, mCircle);
    }
}

#ifndef NDEBUG
void Shape::draw(Color color) const {
    switch (mShape) {
    case ShapeTag::AABB:
        mAABB.draw(color);
        break;
    case ShapeTag::Circle:
        mCircle.draw(color);
        break;
    }
}
#endif

}  // namespace whal

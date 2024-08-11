#include "Shapes.h"

#include <cassert>
#include <cstring>
#include <raylib.h>

#include "Components/Transform.h"
#include "Physics/HitInfo.h"

namespace whal {

Vector2i getRotationCorrection(Vector2i half, f32 rotationDegrees) {
    // -180deg is up
    // -90deg is right
    // 0deg is down
    // 90 deg is left

    // optimize for most common case
    if (rotationDegrees == 0.0) {
        return {0, half.y};
    }
    const f32 correctRadians = DEG2RAD * (rotationDegrees * -1.0f - 90);
    return (Vector2f(-0.5, -1.0) * half.as<f32>() * Vector2f(std::cos(correctRadians), std::sin(correctRadians))).round();
}

Vector2i transToCenter(Transform2D trans, Vector2i half) {
    return trans.position + getRotationCorrection(half, trans.rotationDegrees);
}

Vector2i centerToTrans(Vector2i center, Vector2i half, f32 rotationDegrees) {
    return center - getRotationCorrection(half, rotationDegrees);
}

AABB::AABB(Vector2i center, Vector2i half) : mCenter(center), mHalf(half) {}

AABB::AABB(Transform2D transform, Vector2i half) : mCenter(transToCenter(transform, half)), mHalf(half) {}

void AABB::setPosition(Vector2i center) {
    mCenter = center;
}

void AABB::setPosition(Transform2D transform) {
    mCenter = transToCenter(transform, mHalf);
}

bool AABB::isOverlapping(const AABB* other) const {
    return isIntersectAABBvsAABB(this, other);
}

bool AABB::isOverlapping(const AABB& other) const {
    return isIntersectAABBvsAABB(this, &other);
}

bool AABB::contains(const AABB& other) const {
    return left() <= other.left() && other.right() <= right() && other.top() <= top() && bottom() <= other.bottom();
}

#ifndef NDEBUG
void AABB::draw(Vector2f cameraPos, Color color) const {
    Vector2f position(left(), bottom());
    Vector2f size = Vector2f(mHalf.x, mHalf.y) * 2;

    // subtract size.y so we draw from bottom left instead of top left
    Vector2f dstPosition = {position.x - cameraPos.x, -1 * position.y + cameraPos.y - size.y};
    DrawRectangleLines(dstPosition.x, dstPosition.y, size.x, size.y, color);
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
        const s32 signX = sign(delta.x);
        const s32 signY = sign(delta.y);
        // Vector2i hitPos(center.x + half.x * signX, center.y + half.y * signY);
        // Vector2i hitDelta(px * signX, py * signY);
        Vector2i hitNormal(signX, signY);
        return HitInfo(hitNormal, true);
    }
    if (px < py) {
        const s32 signX = sign(delta.x);
        // Vector2i hitPos(center.x + half.x * signX, other.center.y);
        // Vector2i hitDelta(px * signX, 0);
        Vector2i hitNormal(signX, 0);
        return HitInfo(hitNormal, true);
    } else {
        const s32 signY = sign(delta.y);
        // Vector2i hitPos(other.center.x, center.y + half.y * signY);
        // Vector2i hitDelta(0, py * signY);
        Vector2i hitNormal(0, signY);
        return HitInfo(hitNormal, true);
    }
}

Vector2i AABB::getPositionEdge(Vector2i unitDir) const {
    return mCenter + mHalf * unitDir;
}

Circle::Circle(Vector2i center, s32 radius) : mCenter(center), mRadius(radius) {}

Circle::Circle(Transform2D transform, s32 radius) : mCenter(transform.position.x, transform.position.y + radius), mRadius(radius) {}

void Circle::setPosition(Vector2i center) {
    mCenter = center;
}

void Circle::setPosition(Transform2D transform) {
    // TODO needs work, is a little off on X axis when sprite is Not rotated about center (which reminds me... should be part of Transform)
    // mCenter = transform.position + Vector2i(0, mRadius);
    mCenter = transToCenter(transform, Vector2i(mRadius, mRadius));
}

#ifndef NDEBUG
void Circle::draw(Vector2f cameraPos, Color color) const {
    Vector2f dstPosition = {mCenter.x - cameraPos.x, -1 * mCenter.y + cameraPos.y};
    DrawCircleLines(dstPosition.x, dstPosition.y, mRadius, color);
}
#endif

f32 Circle::getDistanceFromCenter(const AABB* aabb) const {
    const auto delta = getPosition() - aabb->getPosition();  // vector from AABB's center to circle's

    // clamp to be on the AABB's boundary. Is now the point on the AABB closest to the circle
    const auto half = aabb->getHalf();
    const auto closestPoint = aabb->getPosition() + Vector2i(clamp(delta.x, -half.x, half.x), clamp(delta.y, -half.y, half.y));
    return (getPosition() - closestPoint).as<f32>().len();
}

f32 Circle::getDistanceFromCenter(const Circle* other) const {
    return (getPosition() - other->getPosition()).as<f32>().len() - other->getRadius();
}

// calculates a vector from the circle's origin to the closest point on the given AABB
Vector2f Circle::getVecToClosestPoint(const AABB aabb) const {
    const auto delta = getPosition() - aabb.getPosition();  // vector from AABB's center to circle's

    // clamp to be on the AABB's boundary. Is now the point on the AABB closest to the circle
    const auto half = aabb.getHalf();
    const auto closestPoint = aabb.getPosition() + Vector2i(clamp(delta.x, -half.x, half.x), clamp(delta.y, -half.y, half.y));
    return (closestPoint - getPosition()).as<f32>();
}

AABB Circle::getBoundingBox() const {
    return AABB(mCenter, {mRadius, mRadius});
}

bool isIntersectAABBvsAABB(const AABB* first, const AABB* other) {
    const auto delta = other->getPosition() - first->getPosition();
    const auto overlap = first->getHalf() + other->getHalf();
    return overlap.x > abs(delta.x) && overlap.y > abs(delta.y);
}

bool isIntersectCirclevsCircle(const Circle* first, const Circle* other) {
    return first->getDistanceFromCenter(other) <= first->getRadius();
}

bool isIntersectAABBvsCircle(const AABB* aabb, const Circle* circle) {
    return circle->getDistanceFromCenter(aabb) <= circle->getRadius();
}

Shape::Shape(AABB aabb) : mAABB(aabb), mShape(ShapeTag::AABB) {}

Shape::Shape(Circle circle) : mCircle(circle), mShape(ShapeTag::Circle) {}

Shape::Shape(const Shape& other) {
    std::memcpy(this, &other, sizeof(other));
}

Shape& Shape::operator=(const Shape& other) {
    if (this == &other) {
        return *this;
    }
    std::memcpy(this, &other, sizeof(other));
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

void Shape::setPosition(Transform2D transform) {
    switch (mShape) {
    case ShapeTag::AABB:
        mAABB.setPosition(transform);
        break;
    case ShapeTag::Circle:
        mCircle.setPosition(transform);
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
        return isIntersectAABBvsAABB(&mAABB, &other);
    case ShapeTag::Circle:
        return isIntersectAABBvsCircle(&other, &mCircle);
    }
}

bool Shape::isOverlapping(const Circle& other) const {
    switch (mShape) {
    case ShapeTag::AABB:
        return isIntersectAABBvsCircle(&mAABB, &other);
    case ShapeTag::Circle:
        return isIntersectCirclevsCircle(&other, &mCircle);
    }
}

#ifndef NDEBUG
void Shape::draw(Vector2f cameraPos, Color color) const {
    switch (mShape) {
    case ShapeTag::AABB:
        mAABB.draw(cameraPos, color);
        break;
    case ShapeTag::Circle:
        mCircle.draw(cameraPos, color);
        break;
    }
}
#endif

}  // namespace whal

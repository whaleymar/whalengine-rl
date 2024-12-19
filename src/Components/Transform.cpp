#include "Transform.h"

#include "Gfx/Color.h"
#include "Gfx/Coordinates.h"
#include "Gfx/RaylibUtil.h"
#include "Settings.h"
#include "Util/CameraUtil.h"

namespace whal {

static Vector2f _getRotatedPosition(Vector2f position, Vector2f scale, Vector2f pivotOffset, f32 rotationDegrees, f32 floatHeight) {
    if (rotationDegrees == 0.0f) {
        return position + Vector2f(0, floatHeight * FLOAT_HEIGHT_MULT) + pivotOffset * (Vector2f::ONE - scale);
    }
    const Vector2f pivotRoot = position + pivotOffset;
    const Vector2f unscaled = position.rotate(rotationDegrees, pivotRoot) + Vector2f(0, floatHeight * FLOAT_HEIGHT_MULT);
    const Vector2f delta = unscaled - pivotRoot;
    return pivotRoot + delta * scale;
}

Transform Transform::world(s32 x, s32 y) {
    Vector2f pos(x, y);
    return Transform{.position = pos, .positionPx = pos.as<s32>(), .localPosition = pos};
}

Transform Transform::world(Vector2i pos) {
    return Transform{.position = pos.as<f32>(), .positionPx = pos, .localPosition = pos.as<f32>()};
}

Transform Transform::world(Vector2f pos) {
    return Transform{.position = pos, .positionPx = pos.round(), .localPosition = pos};
}

Transform Transform::tiles(s32 x, s32 y) {
    Vector2f pos(x * PIXELS_PER_TILE, y * PIXELS_PER_TILE);
    return Transform{.position = pos, .positionPx = pos.as<s32>(), .localPosition = pos};
}

Transform Transform::tiles(Vector2i pos) {
    Vector2f posF = Vector2f(pos.x * PIXELS_PER_TILE, pos.y * PIXELS_PER_TILE);
    return Transform{.position = posF, .positionPx = posF.as<s32>(), .localPosition = posF};
}

void Transform::translate(Vector2f moveAmount, ecs::Entity self) {
    position += moveAmount;
    positionPx = position.round();
    localPosition += moveAmount;
    for (const ecs::Entity& child : self.children()) {
        child.get<Transform>().setParentPosition(position, child);
    }
}

void Transform::rotate(f32 degrees, ecs::Entity self) {
    rotation += degrees;
    localRotation += degrees;
    for (const ecs::Entity& child : self.children()) {
        child.get<Transform>().setParentRotation(rotation, child);
    }
}

void Transform::scaleBy(Vector2f amount, ecs::Entity self) {
    scale *= amount;
    localScale *= amount;
    for (const ecs::Entity& child : self.children()) {
        child.get<Transform>().setParentScale(scale, child);
    }
}

void Transform::setParent(const Transform& parentTrans, ecs::Entity self) {
    position = parentTrans.position + localPosition;
    positionPx = position.round();
    scale = parentTrans.scale * localScale;
    rotation = parentTrans.rotation + localRotation;
    for (const ecs::Entity& child : self.children()) {
        child.get<Transform>().setParent(*this, child);
    }
}

void Transform::setParentPosition(Vector2f parentPosition, ecs::Entity self) {
    position = parentPosition + localPosition;
    positionPx = position.round();
    for (const ecs::Entity& child : self.children()) {
        child.get<Transform>().setParentPosition(position, child);
    }
}

void Transform::setParentScale(Vector2f parentScale, ecs::Entity self) {
    scale = parentScale * localScale;
    for (const ecs::Entity& child : self.children()) {
        child.get<Transform>().setParentScale(scale, child);
    }
}

void Transform::setParentRotation(f32 parentDegrees, ecs::Entity self) {
    rotation = parentDegrees + localRotation;
    for (const ecs::Entity& child : self.children()) {
        child.get<Transform>().setParentRotation(rotation, child);
    }
}

void Transform::set(const Transform& trans, ecs::Entity self) {
    const Vector2f parentPosition = position - localPosition;
    position = trans.position;
    positionPx = trans.positionPx;
    localPosition = position - parentPosition;

    const Vector2f parentScale = scale / localScale;
    scale = trans.scale;
    localScale = scale / parentScale;

    const f32 parentRotation = rotation - localRotation;
    rotation = trans.rotation;
    localRotation = rotation - parentRotation;

    floatHeight = trans.floatHeight;
    facing = trans.facing;
    // isManuallyMoved = true;
    depth = trans.depth;
    pivotOffset = trans.pivotOffset;

    for (const ecs::Entity& child : self.children()) {
        child.get<Transform>().setParent(*this, child);
    }
}

void Transform::setPosition(Vector2f globalPosition, ecs::Entity self) {
    if (globalPosition == position) {
        return;
    }

    const Vector2f parentPosition = position - localPosition;
    position = globalPosition;
    positionPx = position.round();
    localPosition = position - parentPosition;

    // isManuallyMoved = true;
    for (const ecs::Entity& child : self.children()) {
        child.get<Transform>().setParentPosition(position, child);
    }
}

void Transform::setScale(Vector2f globalScale, ecs::Entity self) {
    if (globalScale == scale) {
        return;
    }

    const Vector2f parentScale = scale / localScale;
    scale = globalScale;
    localScale = scale / parentScale;

    // isManuallyMoved = true;
    for (const ecs::Entity& child : self.children()) {
        child.get<Transform>().setParentScale(scale, child);
    }
}

void Transform::setRotation(f32 globalRotation, ecs::Entity self) {
    if (globalRotation == rotation) {
        return;
    }

    const f32 parentRotation = rotation - localRotation;
    rotation = globalRotation;
    localRotation = rotation - parentRotation;

    // isManuallyMoved = true;
    for (const ecs::Entity& child : self.children()) {
        child.get<Transform>().setParentRotation(rotation, child);
    }
}

Vector2f Transform::getRotatedPosition() const {
    return _getRotatedPosition(position, scale, pivotOffset, rotation, floatHeight);
}

Vector2i Transform::getRotatedPositionInt() const {
    return _getRotatedPosition(positionPx.as<f32>(), scale, pivotOffset, rotation, floatHeight).round();
}

Vector2f Transform::apply(Vector2f relOffset) const {
    // optimize for most common case
    if (rotation == 0.0) {
        const auto scaleAdjustment = (pivotOffset * (Vector2f::ONE - scale));
        return position + relOffset + scaleAdjustment;
    }

    // RESEARCH might want to use fast variants of these functions
    const Vector2f rotatedOffset = relOffset.isZero() ? Vector2f::ZERO : relOffset.as<f32>().rotate(rotation, Vector2f::ZERO);
    return getRotatedPosition() + rotatedOffset;
}

Vector2i Transform::apply(Vector2i relOffset) const {
    // optimize for most common case
    if (rotation == 0.0) {
        const auto scaleAdjustment = (pivotOffset * (Vector2f::ONE - scale)).round();
        return positionPx + relOffset + scaleAdjustment;
    }

    // RESEARCH might want to use fast variants of these functions
    const Vector2i rotatedOffset = relOffset.isZero() ? Vector2i::ZERO : relOffset.as<f32>().rotate(rotation, Vector2f::ZERO).round();
    return getRotatedPositionInt() + rotatedOffset;
}

Vector2f Transform::applyInverse(Vector2f transformedPosition, Vector2f relOffset) const {
    // optimize for most common case
    if (rotation == 0.0) {
        const auto scaleAdjustment = (pivotOffset * (Vector2f::ONE - scale));
        return transformedPosition - relOffset - scaleAdjustment;
    }

    // RESEARCH might want to use fast variants of these functions
    const Vector2f rotatedOffset = relOffset.isZero() ? Vector2f::ZERO : relOffset.as<f32>().rotate(rotation, Vector2f::ZERO);
    const Vector2f transformation = getRotatedPosition() + rotatedOffset;
    return transformedPosition - (transformation - this->position);
}

Vector2i Transform::applyInverse(Vector2i transformedPosition, Vector2i relOffset) const {
    // optimize for most common case
    if (rotation == 0.0) {
        const auto scaleAdjustment = (pivotOffset * (Vector2f::ONE - scale)).round();
        return transformedPosition - relOffset - scaleAdjustment;
    }

    // RESEARCH might want to use fast variants of these functions
    const Vector2i rotatedOffset = relOffset.isZero() ? Vector2i::ZERO : relOffset.as<f32>().rotate(rotation, Vector2f::ZERO).round();
    const Vector2i transformation = getRotatedPositionInt() + rotatedOffset;
    return transformedPosition - (transformation - this->positionPx);
}

#ifndef NDEBUG
// Draws the root position + transformed root, according to the rotation + scale + pivot
void Transform::draw() const {
    const Vector2f cameraPos = getCameraPositionPrecise();
    gfx::DrawPixel(worldToScreenCoords(position, cameraPos).as<f32>(), Colors::Red);
    auto unrounded = _getRotatedPosition(position, scale, pivotOffset, rotation, 0.0f);
    gfx::DrawPixel(worldToScreenCoords(unrounded, cameraPos).as<f32>(), Colors::Green);
}
#endif

TransformBuilder::TransformBuilder(const Transform& trans) : mTrans(trans) {}

TransformBuilder& TransformBuilder::translate(Vector2f moveAmount) {
    mTrans.position += moveAmount;
    mTrans.localPosition += moveAmount;
    mTrans.positionPx = mTrans.position.round();
    return *this;
}

TransformBuilder& TransformBuilder::scaleBy(Vector2f mult) {
    mTrans.scale *= mult;
    mTrans.localScale *= mult;
    return *this;
}

TransformBuilder& TransformBuilder::rotate(f32 degrees) {
    mTrans.rotation += degrees;
    mTrans.localRotation += degrees;
    return *this;
}

TransformBuilder& TransformBuilder::position(Vector2f globalPosition) {
    const Vector2f parentPosition = mTrans.position - mTrans.localPosition;
    mTrans.position = globalPosition;
    mTrans.positionPx = mTrans.position.round();
    mTrans.localPosition = mTrans.position - parentPosition;
    return *this;
}

TransformBuilder& TransformBuilder::scale(Vector2f globalScale) {
    const Vector2f parentScale = mTrans.scale / mTrans.localScale;
    mTrans.scale = globalScale;
    mTrans.localScale = mTrans.scale / parentScale;
    return *this;
}

TransformBuilder& TransformBuilder::rotation(f32 globalRotation) {
    const f32 parentRotation = mTrans.rotation - mTrans.localRotation;
    mTrans.rotation = globalRotation;
    mTrans.localRotation = mTrans.rotation - parentRotation;
    return *this;
}

TransformBuilder& TransformBuilder::height(f32 height) {
    mTrans.floatHeight = height;
    return *this;
}

TransformBuilder& TransformBuilder::depth(Depth depth) {
    mTrans.depth = depth;
    return *this;
}
TransformBuilder& TransformBuilder::facing(Facing facing) {
    mTrans.facing = facing;
    return *this;
}

Transform TransformBuilder::build() const {
    return mTrans;
}

}  // namespace whal

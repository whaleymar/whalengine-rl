#include "Transform.h"

#include "Gfx/Color.h"
#include "Gfx/Coordinates.h"
#include "Gfx/RaylibUtil.h"
#include "Settings.h"
#include "Util/CameraUtil.h"

namespace whal {

static Vector2f _getRotatedPosition(Vector2f position, Vector2f scale, Vector2f pivotOffset, f32 rotationDegrees, f32 floatHeight) {
    if (rotationDegrees == 0.0f) {
        return position + Vector2f(0, floatHeight * FLOAT_HEIGHT_MULT) + pivotOffset.as<f32>() * (Vector2f::ONE - scale);
    }
    const auto pivotRoot = position + pivotOffset.as<f32>();
    const auto unscaled = position.rotate(rotationDegrees, pivotRoot) + Vector2f(0, floatHeight * FLOAT_HEIGHT_MULT);
    const auto delta = unscaled - pivotRoot;
    return pivotRoot + delta * scale;
}

UltimateTransformFinal UltimateTransformFinal::pixels(s32 x, s32 y) {
    return UltimateTransformFinal{.position = Vector2f(x, y), .positionPx = {x, y}};
}

UltimateTransformFinal UltimateTransformFinal::tiles(s32 x, s32 y) {
    return UltimateTransformFinal{.position = Vector2f(x * PIXELS_PER_TILE, y * PIXELS_PER_TILE),
                                  .positionPx = {x * PIXELS_PER_TILE, y * PIXELS_PER_TILE}};
}

void UltimateTransformFinal::translate(Vector2f moveAmount, ecs::Entity self) {
    position += moveAmount;
    positionPx = position.round();
    localPosition += moveAmount;
    for (const ecs::Entity& child : self.children()) {
        child.get<UltimateTransformFinal>().setParentPosition(position, child);
    }
}

void UltimateTransformFinal::rotate(f32 degrees, ecs::Entity self) {
    rotation += degrees;
    localRotation += degrees;
    for (const ecs::Entity& child : self.children()) {
        child.get<UltimateTransformFinal>().setParentRotation(rotation, child);
    }
}

void UltimateTransformFinal::scaleBy(Vector2f amount, ecs::Entity self) {
    scale *= amount;
    localScale *= amount;
    for (const ecs::Entity& child : self.children()) {
        child.get<UltimateTransformFinal>().setParentScale(scale, child);
    }
}

void UltimateTransformFinal::setParent(const UltimateTransformFinal& parentTrans, ecs::Entity self) {
    position = parentTrans.position + localPosition;
    positionPx = position.round();
    scale = parentTrans.scale * localScale;
    rotation = parentTrans.rotation + localRotation;
    for (const ecs::Entity& child : self.children()) {
        child.get<UltimateTransformFinal>().setParent(*this, child);
    }
}

void UltimateTransformFinal::setParentPosition(Vector2f parentPosition, ecs::Entity self) {
    position = parentPosition + localPosition;
    positionPx = position.round();
    for (const ecs::Entity& child : self.children()) {
        child.get<UltimateTransformFinal>().setParentPosition(position, child);
    }
}

void UltimateTransformFinal::setParentScale(Vector2f parentScale, ecs::Entity self) {
    scale = parentScale * localScale;
    for (const ecs::Entity& child : self.children()) {
        child.get<UltimateTransformFinal>().setParentScale(scale, child);
    }
}

void UltimateTransformFinal::setParentRotation(f32 parentDegrees, ecs::Entity self) {
    rotation = parentDegrees + localRotation;
    for (const ecs::Entity& child : self.children()) {
        child.get<UltimateTransformFinal>().setParentRotation(rotation, child);
    }
}

void UltimateTransformFinal::set(const UltimateTransformFinal& trans, ecs::Entity self) {
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
    isManuallyMoved = true;
    depth = trans.depth;
    pivotOffset = trans.pivotOffset;

    for (const ecs::Entity& child : self.children()) {
        child.get<UltimateTransformFinal>().setParent(*this, child);
    }
}

void UltimateTransformFinal::setPosition(Vector2f globalPosition, ecs::Entity self) {
    const Vector2f parentPosition = position - localPosition;
    position = globalPosition;
    positionPx = position.round();
    localPosition = position - parentPosition;

    isManuallyMoved = true;
    for (const ecs::Entity& child : self.children()) {
        child.get<UltimateTransformFinal>().setParentPosition(position, child);
    }
}

void UltimateTransformFinal::setScale(Vector2f globalScale, ecs::Entity self) {
    const Vector2f parentScale = scale / localScale;
    scale = globalScale;
    localScale = scale / parentScale;

    isManuallyMoved = true;
    for (const ecs::Entity& child : self.children()) {
        child.get<UltimateTransformFinal>().setParentScale(scale, child);
    }
}

void UltimateTransformFinal::setRotation(f32 globalRotation, ecs::Entity self) {
    const f32 parentRotation = rotation - localRotation;
    rotation = globalRotation;
    localRotation = rotation - parentRotation;

    isManuallyMoved = true;
    for (const ecs::Entity& child : self.children()) {
        child.get<UltimateTransformFinal>().setParentRotation(rotation, child);
    }
}

Vector2f UltimateTransformFinal::getRotatedPosition() const {
    return _getRotatedPosition(position, scale, pivotOffset.as<f32>(), rotation, floatHeight);
}

Vector2f UltimateTransformFinal::apply(Vector2f relOffset) const {
    // optimize for most common case
    if (rotation == 0.0) {
        const auto scaleAdjustment = (pivotOffset.as<f32>() * (Vector2f::ONE - scale));
        return position + relOffset + scaleAdjustment;
    }

    // RESEARCH might want to use fast variants of these functions
    const Vector2f rotatedOffset = relOffset.isZero() ? Vector2f::ZERO : relOffset.as<f32>().rotate(rotation, Vector2f::ZERO);
    return getRotatedPosition() + rotatedOffset;
}

Vector2f UltimateTransformFinal::applyInverse(Vector2f transformedPosition, Vector2f relOffset) const {
    // optimize for most common case
    if (rotation == 0.0) {
        const auto scaleAdjustment = (pivotOffset.as<f32>() * (Vector2f::ONE - scale));
        return transformedPosition - relOffset - scaleAdjustment;
    }

    // RESEARCH might want to use fast variants of these functions
    const Vector2f rotatedOffset = relOffset.isZero() ? Vector2f::ZERO : relOffset.as<f32>().rotate(rotation, Vector2f::ZERO);
    const Vector2f transformation = getRotatedPosition() + rotatedOffset;
    return transformedPosition - (transformation - this->position);
}

Transform Transform::pixels(s32 x, s32 y) {
    return Transform({x, y});
}

Transform Transform::tiles(s32 x, s32 y) {
    return Transform({x * PIXELS_PER_TILE, y * PIXELS_PER_TILE});
}

#ifndef NDEBUG
// Draws the root position + transformed root, according to the rotation + scale + pivot
void UltimateTransformFinal::draw() const {
    const Vector2f cameraPos = getCameraPositionPrecise();
    gfx::DrawPixel(worldToScreenCoords(position, cameraPos).as<f32>(), Colors::Red);
    auto unrounded = _getRotatedPosition(position, scale, pivotOffset, rotation, 0.0f);
    gfx::DrawPixel(worldToScreenCoords(unrounded, cameraPos).as<f32>(), Colors::Green);
}
#endif

Vector2i Transform::getRotatedPosition() const {
    return _getRotatedPosition(position.as<f32>(), scale, pivotOffset.as<f32>(), rotationDegrees, 0.0f).round();
}

Vector2i Transform::apply(Vector2i relOffset) const {
    // optimize for most common case
    if (rotationDegrees == 0.0) {
        const auto scaleAdjustment = (pivotOffset.as<f32>() * (Vector2f::ONE - scale)).round();
        return position + relOffset + scaleAdjustment;
    }

    // RESEARCH might want to use fast variants of these functions
    const Vector2i rotatedOffset = relOffset.isZero() ? Vector2i::ZERO : relOffset.as<f32>().rotate(rotationDegrees, Vector2f::ZERO).round();
    return getRotatedPosition() + rotatedOffset;
}

Vector2i Transform::applyInverse(Vector2i transformedPosition, Vector2i relOffset) const {
    // optimize for most common case
    if (rotationDegrees == 0.0) {
        const auto scaleAdjustment = (pivotOffset.as<f32>() * (Vector2f::ONE - scale)).round();
        return transformedPosition - relOffset - scaleAdjustment;
    }

    // RESEARCH might want to use fast variants of these functions
    const Vector2i rotatedOffset = relOffset.isZero() ? Vector2i::ZERO : relOffset.as<f32>().rotate(rotationDegrees, Vector2f::ZERO).round();
    const Vector2i transformation = getRotatedPosition() + rotatedOffset;
    return transformedPosition - (transformation - this->position);
}

#ifndef NDEBUG
// Draws the root position + transformed root, according to the rotation + scale + pivot
void Transform::draw() const {
    const Vector2f cameraPos = getCameraPositionPrecise();
    gfx::DrawPixel(worldToScreenCoords(position.as<f32>(), cameraPos).as<f32>(), Colors::Red);
    auto unrounded = _getRotatedPosition(position.as<f32>(), scale, pivotOffset.as<f32>(), rotationDegrees, 0.0f);
    gfx::DrawPixel(worldToScreenCoords(unrounded, cameraPos).as<f32>(), Colors::Green);
}
#endif

PreciseTransform PreciseTransform::pixels(s32 x, s32 y) {
    return PreciseTransform({static_cast<f32>(x), static_cast<f32>(y)});
}

PreciseTransform PreciseTransform::tiles(s32 x, s32 y) {
    return PreciseTransform({x * FPIXELS_PER_TILE, y * FPIXELS_PER_TILE});
}

Vector2f PreciseTransform::getRotatedPosition() const {
    return _getRotatedPosition(position, scale, pivotOffset.as<f32>(), rotationDegrees, floatHeight);
}

Vector2f PreciseTransform::apply(Vector2f relOffset) const {
    // optimize for most common case
    if (rotationDegrees == 0.0) {
        const auto scaleAdjustment = (pivotOffset.as<f32>() * (Vector2f::ONE - scale));
        return position + relOffset + scaleAdjustment;
    }

    // RESEARCH might want to use fast variants of these functions
    const Vector2f rotatedOffset = relOffset.isZero() ? Vector2f::ZERO : relOffset.as<f32>().rotate(rotationDegrees, Vector2f::ZERO);
    return getRotatedPosition() + rotatedOffset;
}

Vector2f PreciseTransform::applyInverse(Vector2f transformedPosition, Vector2f relOffset) const {
    // optimize for most common case
    if (rotationDegrees == 0.0) {
        const auto scaleAdjustment = (pivotOffset.as<f32>() * (Vector2f::ONE - scale));
        return transformedPosition - relOffset - scaleAdjustment;
    }

    // RESEARCH might want to use fast variants of these functions
    const Vector2f rotatedOffset = relOffset.isZero() ? Vector2f::ZERO : relOffset.as<f32>().rotate(rotationDegrees, Vector2f::ZERO);
    const Vector2f transformation = getRotatedPosition() + rotatedOffset;
    return transformedPosition - (transformation - this->position);
}

}  // namespace whal

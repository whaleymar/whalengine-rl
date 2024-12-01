#include "Transform.h"

#include "Gfx/Color.h"
#include "Gfx/Coordinates.h"
#include "Gfx/RaylibUtil.h"
#include "Settings.h"
#include "Util/CameraUtil.h"

namespace whal {

Transform Transform::pixels(s32 x, s32 y) {
    return Transform({x, y});
}

Transform Transform::tiles(s32 x, s32 y) {
    return Transform({x * PIXELS_PER_TILE, y * PIXELS_PER_TILE});
}

static Vector2f _getRotatedPosition(Vector2f position, Vector2f scale, Vector2f pivotOffset, f32 rotationDegrees, f32 floatHeight) {
    if (rotationDegrees == 0.0f) {
        return position + Vector2f(0, floatHeight * FLOAT_HEIGHT_MULT) + pivotOffset.as<f32>() * (Vector2f::ONE - scale);
    }
    const auto pivotRoot = position + pivotOffset.as<f32>();
    const auto unscaled = position.rotate(rotationDegrees, pivotRoot) + Vector2f(0, floatHeight * FLOAT_HEIGHT_MULT);
    const auto delta = unscaled - pivotRoot;
    return pivotRoot + delta * scale;
}

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
    gfx::DrawPixel(worldToScreenCoords(position.as<f32>(), cameraPos), Colors::Red);
    auto unrounded = _getRotatedPosition(position.as<f32>(), scale, pivotOffset.as<f32>(), rotationDegrees, 0.0f);
    gfx::DrawPixel(worldToScreenCoords(unrounded, cameraPos), Colors::Green);
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

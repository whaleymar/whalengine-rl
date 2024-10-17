#include "Transform.h"

#include "Settings.h"

namespace whal {

Transform2D Transform2D::pixels(s32 x, s32 y) {
    return Transform2D({x, y});
}

Transform2D Transform2D::tiles(s32 x, s32 y) {
    return Transform2D({x * PIXELS_PER_TILE, y * PIXELS_PER_TILE});
}

Vector2i Transform2D::getRotatedPosition() const {
    if (rotationDegrees == 0.0f) {
        return position;
    }
    auto const posF = position.as<f32>();
    return posF.rotate(rotationDegrees, posF + pivotOffset.as<f32>() * scale).round();
}

PreciseTransform2D PreciseTransform2D::pixels(s32 x, s32 y) {
    return PreciseTransform2D({static_cast<f32>(x), static_cast<f32>(y)});
}

PreciseTransform2D PreciseTransform2D::tiles(s32 x, s32 y) {
    return PreciseTransform2D({x * FPIXELS_PER_TILE, y * FPIXELS_PER_TILE});
}

Vector2f PreciseTransform2D::getRotatedPosition() const {
    if (rotationDegrees == 0.0f) {
        return position;
    }
    return position.rotate(rotationDegrees, position + pivotOffset.as<f32>() * scale);
}

}  // namespace whal

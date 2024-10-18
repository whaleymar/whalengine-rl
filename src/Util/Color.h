#pragma once

#include <raylib.h>
#include "Util/MathUtil.h"

inline Color operator+(const Color& left, const Color& right) {
    return Color(left.r + right.r, left.g + right.g, left.b + right.b, left.a + right.a);
}

inline Color operator*=(Color& left, const f32 f) {
    left.r = left.r * f;
    left.g = left.g * f;
    left.b = left.b * f;
    left.a = left.a * f;
    return left;
}

namespace whal {

namespace Colors {

inline static Color Clear = {0, 0, 0, 0};
inline static Color Magenta = {255, 0, 255, 255};
inline static Color Emerald = {80, 204, 96, 255};
inline static Color Purple = {198, 51, 242, 255};
// inline static Color Pink = {242, 116, 217, 255};
inline static Color Pink = {255, 170, 255, 255};
inline static Color LightBlue = {85, 255, 255, 255};

inline Color lerp(Color first, Color second, f32 t) {
    return Color{static_cast<u8>(math::lerp(static_cast<f32>(first.r), static_cast<f32>(second.r), t)),
                 static_cast<u8>(math::lerp(static_cast<f32>(first.g), static_cast<f32>(second.g), t)),
                 static_cast<u8>(math::lerp(static_cast<f32>(first.b), static_cast<f32>(second.b), t)),
                 static_cast<u8>(math::lerp(static_cast<f32>(first.a), static_cast<f32>(second.a), t))};
}

}  // namespace Colors

}  // namespace whal

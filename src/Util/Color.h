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

inline static const Color CLEAR = {0, 0, 0, 0};
inline static const Color EMERALD = {80, 204, 96, 255};
inline static const Color WHAL_PURPLE = {198, 51, 242, 255};
inline static const Color WHAL_PINK = {255, 170, 255, 255};
inline static const Color LIGHT_BLUE = {85, 255, 255, 255};

inline Color lerp(Color first, Color second, f32 t) {
    return Color{static_cast<u8>(math::lerp(static_cast<f32>(first.r), static_cast<f32>(second.r), t)),
                 static_cast<u8>(math::lerp(static_cast<f32>(first.g), static_cast<f32>(second.g), t)),
                 static_cast<u8>(math::lerp(static_cast<f32>(first.b), static_cast<f32>(second.b), t)),
                 static_cast<u8>(math::lerp(static_cast<f32>(first.a), static_cast<f32>(second.a), t))};
}

}  // namespace Colors

}  // namespace whal

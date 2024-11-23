#pragma once

#include <raylib.h>
#include "Util/MathUtil.h"

inline rl::Color operator+(const rl::Color& left, const rl::Color& right) {
    return rl::Color(left.r + right.r, left.g + right.g, left.b + right.b, left.a + right.a);
}

inline rl::Color operator*=(rl::Color& left, const f32 f) {
    left.r = left.r * f;
    left.g = left.g * f;
    left.b = left.b * f;
    left.a = left.a * f;
    return left;
}

namespace whal {

namespace Colors {

inline static const rl::Color CLEAR = {0, 0, 0, 0};
inline static const rl::Color EMERALD = {80, 204, 96, 255};
inline static const rl::Color WHAL_PURPLE = {198, 51, 242, 255};
inline static const rl::Color WHAL_PINK = {255, 170, 255, 255};
inline static const rl::Color LIGHT_BLUE = {85, 255, 255, 255};

inline rl::Color lerp(rl::Color first, rl::Color second, f32 t) {
    return rl::Color{static_cast<u8>(math::lerp(static_cast<f32>(first.r), static_cast<f32>(second.r), t)),
                     static_cast<u8>(math::lerp(static_cast<f32>(first.g), static_cast<f32>(second.g), t)),
                     static_cast<u8>(math::lerp(static_cast<f32>(first.b), static_cast<f32>(second.b), t)),
                     static_cast<u8>(math::lerp(static_cast<f32>(first.a), static_cast<f32>(second.a), t))};
}

}  // namespace Colors

}  // namespace whal

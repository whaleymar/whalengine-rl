#pragma once

#include <raylib.h>
#include "Util/Types.h"

// TODO consolidate with Util/Color.h
namespace whal {

// HDR Color
struct Color {
    f32 r;
    f32 g;
    f32 b;
    f32 a = 1.0f;  // should still be between 0 and 1

    inline rl::Vector4 asRL() const { return rl::Vector4{r, g, b, a}; }

    inline rl::Color asLDR() const {
        f32 len = std::sqrt(r * r + g * g + b * b);
        rl::Vector4 norm = rl::Vector4{r / len, g / len, b / len, a};
        return rl::ColorFromNormalized(norm);
    }

    inline Color operator+(const Color& other) const { return Color{r + other.r, g + other.g, b + other.b, a + other.a}; }
    inline Color operator+=(const Color& other) { return Color{r + other.r, g + other.g, b + other.b, a + other.a}; }
    inline Color operator-(const Color& other) const { return Color{r - other.r, g - other.g, b - other.b, a - other.a}; }
    inline Color operator-=(const Color& other) { return Color{r - other.r, g - other.g, b - other.b, a - other.a}; }
    inline Color operator*(const Color& other) const { return Color{r * other.r, g * other.g, b * other.b, a * other.a}; }
    inline Color operator*=(const Color& other) { return Color{r * other.r, g * other.g, b * other.b, a * other.a}; }
    inline Color operator/(const Color& other) const { return Color{r / other.r, g / other.g, b / other.b, a / other.a}; }
    inline Color operator/=(const Color& other) { return Color{r / other.r, g / other.g, b / other.b, a / other.a}; }
};

inline Color lerp(const Color& lhs, const Color& rhs, const f32 t) {
    return Color{
        math::lerp(lhs.r, rhs.r, t),
        math::lerp(lhs.g, rhs.g, t),
        math::lerp(lhs.b, rhs.b, t),
        math::lerp(lhs.a, rhs.a, t),
    };
}

}  // namespace whal

#pragma once

#include "Util/Types.h"

namespace rl {

typedef struct Vector4 Vector4;
typedef struct Color Color;

// inline rl::Color operator+(const rl::Color& left, const rl::Color& right) {
//     return rl::Color(left.r + right.r, left.g + right.g, left.b + right.b, left.a + right.a);
// }
//
// inline rl::Color operator*=(rl::Color& left, const f32 f) {
//     left.r = left.r * f;
//     left.g = left.g * f;
//     left.b = left.b * f;
//     left.a = left.a * f;
//     return left;
// }

}  // namespace rl

namespace whal {

// HDR Color
struct Color {
    f32 r;
    f32 g;
    f32 b;
    f32 a = 1.0f;  // should still be between 0 and 1

    rl::Vector4 asRL() const;
    rl::Color asLDR() const;

    // brighten or darken a color by some scalar
    // void scale(f32 mod) { return Color{r * mod, g * mod, b * mod, a}; }
    void scale(f32 mod) {
        r *= mod;
        g *= mod;
        b *= mod;
    }

    inline Color operator+(const Color& other) const { return Color{r + other.r, g + other.g, b + other.b, a + other.a}; }
    inline Color operator+=(const Color& other) { return Color{r + other.r, g + other.g, b + other.b, a + other.a}; }
    inline Color operator-(const Color& other) const { return Color{r - other.r, g - other.g, b - other.b, a - other.a}; }
    inline Color operator-=(const Color& other) { return Color{r - other.r, g - other.g, b - other.b, a - other.a}; }
    inline Color operator*(const Color& other) const { return Color{r * other.r, g * other.g, b * other.b, a * other.a}; }
    inline Color operator*=(const Color& other) { return Color{r * other.r, g * other.g, b * other.b, a * other.a}; }
    inline Color operator/(const Color& other) const { return Color{r / other.r, g / other.g, b / other.b, a / other.a}; }
    inline Color operator/=(const Color& other) { return Color{r / other.r, g / other.g, b / other.b, a / other.a}; }

    static Color lerp(const Color& lhs, const Color& rhs, const f32 t);
    static Color fromRL(rl::Color color, f32 brightness = 1.0f);
    static Color fromRGB(s32 r, s32 g, s32 b, s32 a = 255, f32 brightness = 1.0f);
};

namespace Colors {

extern const Color Clear;
extern const Color White;
extern const Color ClearWhite;
extern const Color Black;
extern const Color Emerald;
extern const Color Purple;
extern const Color Pink;
extern const Color LightBlue;
extern const Color Red;
extern const Color Green;
extern const Color DarkGreen;
extern const Color Gray;
extern const Color DarkGray;
extern const Color Brown;
extern const Color DarkBrown;
extern const Color Beige;
extern const Color Blue;
extern const Color DarkBlue;
extern const Color Orange;
extern const Color Magenta;

extern const rl::Color ClearRL;

rl::Color lerp(rl::Color first, rl::Color second, f32 t);

}  // namespace Colors

}  // namespace whal

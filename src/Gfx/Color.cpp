#include "Color.h"

#include <cassert>
#include <ostream>
#include <raylib.h>
#include "Util/MathUtil.h"
#include "Util/Print.h"

namespace whal {

rl::Vector4 Color::asRL() const {
    return rl::Vector4{r, g, b, a};
}

rl::Color Color::asLDR() const {
    f32 max = r > g ? (r > b ? r : b) : (g > b ? g : b);
    rl::Vector4 norm;
    if (max > 1.0f) {
        norm = rl::Vector4{r / max, g / max, b / max, a};
    } else {
        norm = asRL();
    }
    return rl::ColorFromNormalized(norm);
}

Color Color::lerp(const Color& lhs, const Color& rhs, const f32 t) {
    return Color{
        math::lerp(lhs.r, rhs.r, t),
        math::lerp(lhs.g, rhs.g, t),
        math::lerp(lhs.b, rhs.b, t),
        math::lerp(lhs.a, rhs.a, t),
    };
}

Color Color::fromRL(rl::Color color, f32 brightness) {
    rl::Vector4 hdrColor = ColorNormalize(color);  // gets color as floats btwn 0-1
    return Color{hdrColor.x * brightness, hdrColor.y * brightness, hdrColor.z * brightness, hdrColor.w};
}

Color Color::fromRGB(s32 r, s32 g, s32 b, s32 a, f32 brightness) {
    return fromRL(rl::Color{static_cast<u8>(r), static_cast<u8>(g), static_cast<u8>(b), static_cast<u8>(a)}, brightness);
}

Color Color::fromString(const std::string& name) {
    if (name == "Clear") {
        return Colors::Clear;
    } else if (name == "White") {
        return Colors::White;
    } else if (name == "ClearWhite") {
        return Colors::ClearWhite;
    } else if (name == "Black") {
        return Colors::Black;
    } else if (name == "Emerald") {
        return Colors::Emerald;
    } else if (name == "Purple") {
        return Colors::Purple;
    } else if (name == "Pink") {
        return Colors::Pink;
    } else if (name == "LightBlue") {
        return Colors::LightBlue;
    } else if (name == "Red") {
        return Colors::Red;
    } else if (name == "Green") {
        return Colors::Green;
    } else if (name == "DarkGreen") {
        return Colors::DarkGreen;
    } else if (name == "Gray") {
        return Colors::Gray;
    } else if (name == "DarkGray") {
        return Colors::DarkGray;
    } else if (name == "Brown") {
        return Colors::Brown;
    } else if (name == "DarkBrown") {
        return Colors::DarkBrown;
    } else if (name == "Beige") {
        return Colors::Beige;
    } else if (name == "Blue") {
        return Colors::Blue;
    } else if (name == "DarkBlue") {
        return Colors::DarkBlue;
    } else if (name == "Orange") {
        return Colors::Orange;
    } else if (name == "Magenta") {
        return Colors::Magenta;
    } else if (name == "Yellow") {
        return Colors::Yellow;
    } else if (name == "Gold") {
        return Colors::Gold;
    }

    print("No matching color found for", name);
    return Colors::White;
}

Color Color::fromHex(const std::string& hexString, HexStringVariant variant) {
    s32 r, g, b, a;

    switch (variant) {
    case HexStringVariant::RGBA:
        assert(hexString.length() == 8 || hexString.length() == 6);

        std::istringstream(hexString.substr(0, 2)) >> std::hex >> r;
        std::istringstream(hexString.substr(2, 2)) >> std::hex >> g;
        std::istringstream(hexString.substr(4, 2)) >> std::hex >> b;
        if (hexString.length() == 8) {
            std::istringstream(hexString.substr(6, 2)) >> std::hex >> a;
        } else {
            a = 255;
        }

        break;

    case HexStringVariant::ARGB:
        assert(hexString.length() == 8);
        std::istringstream(hexString.substr(0, 2)) >> std::hex >> a;
        std::istringstream(hexString.substr(2, 2)) >> std::hex >> r;
        std::istringstream(hexString.substr(4, 2)) >> std::hex >> g;
        std::istringstream(hexString.substr(6, 2)) >> std::hex >> b;

        break;
    }

    return Color::fromRGB(r, g, b, a);
}

std::ostream& operator<<(std::ostream& out, Color const& self) {
    return out << "(" << self.r << ", " << self.g << ", " << self.b << ", " << self.a << ")";
}

namespace Colors {

const Color Clear = {0, 0, 0, 0};
const Color White = Color{1.0f, 1.0f, 1.0f, 1.0f};
const Color ClearWhite = Color{1.0f, 1.0f, 1.0f, 0.0f};
const Color Black = Color{0.0f, 0.0f, 0.0f, 1.0f};
const Color Emerald = Color::fromRL(rl::Color{80, 204, 96, 255});
const Color Purple = Color::fromRL(rl::Color{198, 51, 242, 255});
const Color Pink = Color::fromRL(rl::Color{255, 170, 255, 255});
const Color LightBlue = Color::fromRL(rl::Color{85, 255, 255, 255});
const Color Red = Color::fromRL(rl::RED);
const Color Green = Color::fromRL(rl::GREEN);
const Color DarkGreen = Color::fromRL(rl::DARKGREEN);
const Color Gray = Color::fromRL(rl::GRAY);
const Color DarkGray = Color::fromRL(rl::DARKGRAY);
const Color Brown = Color::fromRL(rl::BROWN);
const Color DarkBrown = Color::fromRL(rl::DARKBROWN);
const Color Beige = Color::fromRL(rl::BEIGE);
const Color Blue = Color::fromRL(rl::BLUE);
const Color DarkBlue = Color::fromRL(rl::DARKBLUE);
const Color Orange = Color::fromRL(rl::ORANGE);
const Color Magenta = Color::fromRL(rl::MAGENTA);
const Color Yellow = Color::fromRL(rl::YELLOW);
const Color Gold = Color::fromRL(rl::GOLD);

const rl::Color ClearRL = Clear.asLDR();

rl::Color lerp(rl::Color first, rl::Color second, f32 t) {
    return rl::Color{static_cast<u8>(math::lerp(static_cast<f32>(first.r), static_cast<f32>(second.r), t)),
                     static_cast<u8>(math::lerp(static_cast<f32>(first.g), static_cast<f32>(second.g), t)),
                     static_cast<u8>(math::lerp(static_cast<f32>(first.b), static_cast<f32>(second.b), t)),
                     static_cast<u8>(math::lerp(static_cast<f32>(first.a), static_cast<f32>(second.a), t))};
}
}  // namespace Colors

}  // namespace whal

namespace rl {

Color operator+(const Color& left, const Color& right) {
    return Color(left.r + right.r, left.g + right.g, left.b + right.b, left.a + right.a);
}

Color operator*(const Color& left, const Color& right) {
    return Color(left.r * right.r, left.g * right.g, left.b * right.b, left.a * right.a);
}

Color& operator*=(Color& left, const Color& right) {
    left.r *= right.r;
    left.g *= right.g;
    left.b *= right.b;
    left.a *= right.a;
    return left;
}

Color operator*(const Color& left, const f32 f) {
    return Color(left.r * f, left.g * f, left.b * f, left.a * f);
}

Color& operator*=(Color& left, const f32 f) {
    left.r = left.r * f;
    left.g = left.g * f;
    left.b = left.b * f;
    left.a = left.a * f;
    return left;
}

}  // namespace rl

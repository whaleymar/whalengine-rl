#pragma once

#include <cmath>
#include <concepts>
#ifdef __EMSCRIPTEN__
#include <algorithm>  // for std::min/max
#endif
#include "Types.h"

namespace math {

inline constexpr f32 PI_C = 3.14159265358979323846;
inline constexpr f32 RAD_TO_DEG = 180.0f / PI_C;
inline constexpr f32 DEG_TO_RAD = PI_C / 180.0f;

template <class T>
concept Number = std::integral<T> || std::floating_point<T>;

template <class T>
concept SignedNumber = std::signed_integral<T> || std::floating_point<T>;

inline SignedNumber auto abs(SignedNumber auto const number) {
    return number < 0 ? -number : number;
}

inline bool isNearZero(const f32 value, const f32 epsilon) {
    return abs(value) < epsilon;
}

template <std::totally_ordered T>
inline T clamp(const T number, const T min, const T max) {
    if (number < min) {
        return min;
    }
    if (number > max) {
        return max;
    }
    return number;
}

template <std::totally_ordered T>
inline bool isBetween(const T number, const T min, const T max) {
    return number >= min && number <= max;
}

inline SignedNumber auto sign(SignedNumber auto const number) {
    return number < 0 ? -1 : 1;
}

inline f32 remainder(f32 num) {
    f32 unused;
    return std::modf(num, &unused);
}

// inline Number auto lerp(Number auto n1, Number auto n2, f32 t) {
// return (1-t) * n1 + t * n2;
inline f32 lerp(const f32 n1, const f32 n2, const f32 t) {
    return std::lerp(n1, n2, math::clamp(t, 0.0f, 1.0f));
}

inline f32 approach(const f32 val, const f32 target, const f32 move) {
    if (val <= target) {
        return std::min(val + move, target);
    }
    return std::max(val - move, target);
}

// fast constexpr sin/cos using lookup table
// note: marking them constexpr makes linking fail for some reason
f32 fast_sin(f32 radians);
f32 fast_cos(f32 radians);
inline f32 sin(f32 radians) {
    return std::sin(radians);
}
inline f32 cos(f32 radians) {
    return std::cos(radians);
}

// Helper function to normalize an angle to the range [0, 360)
f32 normalizeAngle(f32 angle);

// guaranteed to be in the range [-180, 180]
f32 getAngleDiff(f32 angle1, f32 angle2);

f32 gammaToLinear(f32 gamma);

}  // namespace math

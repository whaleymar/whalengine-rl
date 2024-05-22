#pragma once

#include <algorithm>
#include <cmath>
#include <concepts>
#include "Util/Types.h"
#include "Util/Vector.h"

inline constexpr f32 PI_MINE = 3.14159265358979323846;
inline constexpr f32 RAD_TO_DEG = 180.0f / PI_MINE;

template <class T>
concept SignedNumber = std::signed_integral<T> || std::floating_point<T>;

template <class T>
concept Number = std::integral<T> || std::floating_point<T>;

inline SignedNumber auto abs(SignedNumber auto const number) {
    return number < 0 ? -number : number;
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

// these are not working
template <std::floating_point T>
inline bool isNearZero(const T value, const T epsilon = 0.001) {
    return abs(value) < epsilon;
}

inline bool isNearZerof(const f32 value, const f32 epsilon) {
    return abs(value) < epsilon;
}

// inline Number auto lerp(Number auto n1, Number auto n2, f32 t) {
// return (1-t) * n1 + t * n2;
inline f32 lerp(const f32 n1, const f32 n2, const f32 t) {
    return std::lerp(n1, n2, clamp(t, 0.0f, 1.0f));
}

inline whal::Vector2f lerp(const whal::Vector2f vec1, const whal::Vector2f vec2, const f32 t) {
    return whal::Vector2f(lerp(vec1.x(), vec2.x(), t), lerp(vec1.y(), vec2.y(), t));
}

inline f32 approach(const f32 val, const f32 target, const f32 move) {
    if (val <= target) {
        return std::min(val + move, target);
    }
    return std::max(val - move, target);
}

inline f32 easeInOutBezier(const f32 n1, const f32 n2, const f32 t) {
    return lerp(n1, n2, t * t * (3 - 2 * t));
}

inline f32 easeInOutBezierDt(const f32 n1, const f32 n2, const f32 t) {
    return lerp(n1, n2, -6 * (t - 1) * t);
}

inline f32 easeInOutSine(const f32 n1, const f32 n2, const f32 t) {
    // trough @ t=0, crest @ t=1
    return lerp(n1, n2, 0.5 * (1 + std::sin(PI_MINE * (t - 0.5))));
}

inline f32 easeInOutSineDt(const f32 n1, const f32 n2, const f32 t) {
    // trough @ t=0, crest @ t=0.5, trough @ t=1
    return lerp(n1, n2, 0.5 * PI_MINE * std::cos(PI_MINE * (t - 0.5)));
}

inline f32 easeInQuad(const f32 n1, const f32 n2, const f32 t) {
    return lerp(n1, n2, t * t);
}

inline f32 easeInQuadDt(const f32 n1, const f32 n2, const f32 t) {
    return lerp(n1, n2, 2 * t);
}

inline f32 easeInCubic(const f32 n1, const f32 n2, const f32 t) {
    return lerp(n1, n2, t * t * t);
}

inline f32 easeInCubicDt(const f32 n1, const f32 n2, const f32 t) {
    return lerp(n1, n2, 3 * t * t);
}

inline f32 easeOutQuad(const f32 n1, const f32 n2, const f32 t) {
    return lerp(n1, n2, std::sqrt(t));
}

inline f32 easeOutQuadDt(const f32 n1, const f32 n2, const f32 t) {
    return lerp(n1, n2, 1 / (2 * std::sqrt(t)));
}

inline f32 easeOutCubic(const f32 n1, const f32 n2, const f32 t) {
    return lerp(n1, n2, std::cbrt(t));
}

inline f32 easeOutCubicDt(const f32 n1, const f32 n2, const f32 t) {
    return lerp(n1, n2, 1 / (3 * std::cbrt(t * t)));
}

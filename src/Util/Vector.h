#pragma once

#include <cassert>
#include <iosfwd>
#include <raylib.h>

#include "Util/MathUtil.h"
#include "Util/Types.h"

template <typename T>
struct Vector2 {
    T x = 0;
    T y = 0;

    Vector2() = default;
    Vector2(T elem) : x(elem), y(elem) {}
    Vector2(T elem1, T elem2) : x(elem1), y(elem2) {}
    Vector2(const Vector2<T>& other) = default;
    Vector2(rl::Vector2 rlVec) : x(rlVec.x), y(rlVec.y) {}

    static const Vector2<T> UP;
    static const Vector2<T> DOWN;
    static const Vector2<T> LEFT;
    static const Vector2<T> RIGHT;
    static const Vector2<T> ZERO;
    static const Vector2<T> ONE;

    inline Vector2<T>& operator=(const Vector2<T>& other) {
        x = other.x;
        y = other.y;
        return *this;
    }

    inline Vector2<T> operator+(const Vector2<T> other) const { return Vector2<T>(x + other.x, y + other.y); }
    inline Vector2<T>& operator+=(const Vector2<T> other) {
        x += other.x;
        y += other.y;
        return *this;
    }
    inline Vector2<T> operator-(const Vector2<T> other) const { return Vector2<T>(x - other.x, y - other.y); }
    inline Vector2<T>& operator-=(const Vector2<T> other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }
    inline Vector2<T> operator*(const Vector2<T> other) const { return Vector2<T>(x * other.x, y * other.y); }
    inline Vector2<T>& operator*=(const Vector2<T> other) {
        x *= other.x;
        y *= other.y;
        return *this;
    }
    inline Vector2<T> operator*(const f32 scalar) const { return Vector2<T>(x * scalar, y * scalar); }
    inline Vector2<T>& operator*=(const f32 scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    inline Vector2<T> operator/(const Vector2<T> other) const {
        assert(other.x != 0 && other.y != 0 && "Divide By Zero Error");
        return Vector2<T>(x / other.x, y / other.y);
    }
    inline Vector2<T>& operator/=(const Vector2<T> other) {
        assert(other.x != 0 && other.y != 0 && "Divide By Zero Error");
        x /= other.x;
        y /= other.y;
        return *this;
    }
    inline Vector2<T> operator/(const f32 scalar) const {
        assert(scalar != 0 && "Divide By Zero Error");
        return Vector2<T>(x / scalar, y / scalar);
    }
    inline Vector2<T>& operator/=(const f32 scalar) {
        assert(scalar != 0 && "Divide By Zero Error");
        x /= scalar;
        y /= scalar;
        return *this;
    }

    inline bool operator==(const Vector2<T> other) const { return x == other.x && y == other.y; }

    inline T dot(const Vector2<T> other) const { return x * other.x + y * other.y; }
    inline T det(const Vector2<T> other) const { return x * other.y - y * other.x; }
    inline T len() const { return std::sqrt(x * x + y * y); }
    inline Vector2<T> norm() const {
        assert(!isZero() && "Cannot take norm of zero-length vector");
        T divisor = 1 / len();
        return Vector2<T>(x * divisor, y * divisor);
    }

    inline Vector2<T> absolute() const { return Vector2<T>(math::abs(x), math::abs(y)); }
    inline bool isZero() const { return x == 0 && y == 0; }
    inline bool isNearZero(const f32 epsilon) const { return math::isNearZero(x, epsilon) && math::isNearZero(y, epsilon); }

    template <typename Type>
    inline Vector2<Type> as() const {
        return Vector2<Type>(static_cast<Type>(x), static_cast<Type>(y));
    }

    // Converts to raylib Vector2 struct
    inline rl::Vector2 asRL() const { return rl::Vector2{static_cast<f32>(x), static_cast<f32>(y)}; }

    inline Vector2<s32> round() const { return Vector2<s32>(std::roundf(x), std::roundf(y)); }
    inline Vector2<s32> ceil() const { return Vector2<s32>(std::ceil(x), std::ceil(y)); }

    inline Vector2<T> clamp(T lower, T upper) const { return Vector2<T>(math::clamp(x, lower, upper), math::clamp(y, lower, upper)); }
    inline Vector2<T> clamp(Vector2<T> lower, Vector2<T> upper) const {
        return Vector2<T>(math::clamp(x, lower.x, upper.x), math::clamp(y, lower.y, upper.y));
    }

    // from https://stackoverflow.com/questions/2259476/rotating-a-point-about-another-point-2d
    inline Vector2<T> rotate(f32 angleDegrees, Vector2<T> about) const {
        if (math::isNearZero(angleDegrees, 0.1f)) {
            return *this;
        }
        const f32 radians = -math::DEG_TO_RAD * angleDegrees;
        const f32 sin = math::sin(radians);
        const f32 cos = math::cos(radians);

        // translate point back to origin:
        f32 originX = x - about.x;
        f32 originY = y - about.y;

        // rotate point:
        const f32 xnew = originX * cos - originY * sin;
        const f32 ynew = originX * sin + originY * cos;

        // translate point back:
        return Vector2<T>(xnew + about.x, ynew + about.y);
    }

    // Returns angle of vector.
    // Inputs do not need to be normalized
    inline f32 angle(Vector2<T> reference = Vector2<T>::RIGHT, const bool clockwise = false) const {
        Vector2<f32> vec;
        if constexpr (std::is_same_v<T, f32>) {
            vec = norm();
        } else {
            vec = as<f32>().norm();
        }
        reference = reference.norm();

        const f32 dot = vec.dot(reference.as<f32>());
        const f32 det = vec.det(reference.as<f32>());
        const f32 mult = clockwise ? 1.0f : -1.0f;
        const f32 angleRadians = mult * std::atan2(det, dot);
        const f32 angleDegrees = angleRadians * math::RAD_TO_DEG;

        // angles >180 are negative. clamp between 0 and 360
        if (angleDegrees < 0.0f) {
            return 360.0f + angleDegrees;
        }
        return angleDegrees;
    }

    // Returns unit vector for given angle.
    // Only recommended for float specialization.
    inline static Vector2<T> fromAngle(f32 angle) {
        const f32 radians = angle * DEG2RAD;
        return Vector2<T>(math::cos(radians), math::sin(radians));
    }

    // Returns unit vector for given angle.
    // Only recommended for float specialization.
    inline static Vector2<T> fromAngleFast(f32 angle) {
        const f32 radians = angle * DEG2RAD;
        return Vector2<T>(math::fast_cos(radians), math::fast_sin(radians));
    }

    inline Vector2<T> lerp(const Vector2<T> other, const f32 t) const {
        if constexpr (std::is_same_v<T, f32> || std::is_same_v<T, f64>) {
            return {math::lerp(x, other.x, t), math::lerp(y, other.y, t)};
        } else {
            return as<f32>().lerp(other.as<f32>(), t).round().template as<T>();
        }
    }

    // reflect-cpp compatibility:
    struct POD {
        T x;
        T y;
    };
    using ReflectionType = POD;

    Vector2(POD pod) : x(pod.x), y(pod.y) {}
    const ReflectionType reflection() const { return POD{.x = x, .y = y}; }
};

template <typename T>
std::ostream& operator<<(std::ostream& out, Vector2<T> const& self);

#define DECL_VECTOR_STATICS(type)                                                                                                                    \
    template <>                                                                                                                                      \
    const Vector2<type> Vector2<type>::UP;                                                                                                           \
    template <>                                                                                                                                      \
    const Vector2<type> Vector2<type>::DOWN;                                                                                                         \
    template <>                                                                                                                                      \
    const Vector2<type> Vector2<type>::LEFT;                                                                                                         \
    template <>                                                                                                                                      \
    const Vector2<type> Vector2<type>::RIGHT;                                                                                                        \
    template <>                                                                                                                                      \
    const Vector2<type> Vector2<type>::ZERO;                                                                                                         \
    template <>                                                                                                                                      \
    const Vector2<type> Vector2<type>::ONE;

DECL_VECTOR_STATICS(s8)
DECL_VECTOR_STATICS(s16)
DECL_VECTOR_STATICS(s32)
DECL_VECTOR_STATICS(s64)
DECL_VECTOR_STATICS(f32)
DECL_VECTOR_STATICS(f64)

typedef Vector2<f32> Vector2f;
typedef Vector2<s32> Vector2i;

// lets me use Vector2i as a hashmap key
struct Vector2iHash {
    size_t operator()(const Vector2i& v) const { return (v.y << 16) ^ v.x; }
};

#pragma once

#include <cassert>
#include <iosfwd>

#include "Util/MathUtil.h"
#include "Util/Types.h"

typedef struct Vector2 Vector2;

template <typename T>
struct Vector2T {
    T x, y;

    Vector2T() : x(0), y(0) {}

    Vector2T(T elem1, T elem2) : x(elem1), y(elem2) {}

    Vector2T(const Vector2T<T>& other) : x(other.x), y(other.y) {}

    static inline Vector2T<T> unitUp{0, 1};
    static inline Vector2T<T> unitDown{0, -1};
    static inline Vector2T<T> unitLeft{-1, 0};
    static inline Vector2T<T> unitRight{1, 0};
    static inline Vector2T<T> zero{0, 0};
    static inline Vector2T<T> one{1, 1};

    inline Vector2T<T>& operator=(const Vector2T<T>& other) {
        x = other.x;
        y = other.y;
        return *this;
    }

    inline Vector2T<T> operator+(const Vector2T<T> other) const { return Vector2T<T>(x + other.x, y + other.y); }
    inline Vector2T<T> operator+=(const Vector2T<T> other) {
        x += other.x;
        y += other.y;
        return *this;
    }
    inline Vector2T<T> operator-(const Vector2T<T> other) const { return Vector2T<T>(x - other.x, y - other.y); }
    inline Vector2T<T> operator-=(const Vector2T<T> other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }
    inline Vector2T<T> operator*(const Vector2T<T> other) const { return Vector2T<T>(x * other.x, y * other.y); }
    inline Vector2T<T> operator*=(const Vector2T<T> other) {
        x *= other.x;
        y *= other.y;
        return *this;
    }
    inline Vector2T<T> operator*(const f32 scalar) const { return Vector2T<T>(x * scalar, y * scalar); }
    inline Vector2T<T> operator*=(const f32 scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    inline Vector2T<T> operator/(const f32 scalar) const {
        assert(scalar != 0 && "Divide By Zero Error");
        return Vector2T<T>(x / scalar, y / scalar);
    }
    inline Vector2T<T> operator/=(const f32 scalar) {
        assert(scalar != 0 && "Divide By Zero Error");
        x /= scalar;
        y /= scalar;
        return *this;
    }

    inline bool operator==(const Vector2T<T> other) const { return x == other.x && y == other.y; }

    inline T dot(const Vector2T<T> other) const { return x * other.x + y * other.y; }
    inline T det(const Vector2T<T> other) const { return x * other.y - y * other.x; }
    inline T len() const { return std::sqrt(x * x + y * y); }
    inline Vector2T<T> norm() const {
        assert(!isZero() && "Cannot take norm of zero-length vector");
        T divisor = 1 / len();
        return Vector2T<T>(x * divisor, y * divisor);
    }

    inline Vector2T<T> absolute() const { return Vector2T<T>(math::abs(x), math::abs(y)); }
    inline bool isZero() const { return x == 0 && y == 0; }

    template <typename Type>
    Vector2T<Type> as() const {
        return Vector2T<Type>(static_cast<Type>(x), static_cast<Type>(y));
    }

    Vector2T<s32> round() const { return Vector2T<s32>(std::roundf(x), std::roundf(y)); }
};

template <typename T>
std::ostream& operator<<(std::ostream& out, Vector2T<T> const& self);

typedef Vector2T<f32> Vector2f;
typedef Vector2T<s32> Vector2i;

Vector2i toIntVecRounded(const Vector2f floatVec);
Vector2f fromRaylib(Vector2 rlVec);
Vector2i fromRaylibInt(Vector2 rlVec);
Vector2 toRaylib(Vector2i vec);
Vector2 toRaylib(Vector2f vec);

Vector2f angleToUnit(f32 angle);
Vector2f angleToUnitFast(f32 angle);

// inputs do not need to be normalized
f32 getAngleClockwise(Vector2f vec, Vector2f reference = Vector2f::unitRight);

// counter clockwise (like unit circle)
f32 getAngle(Vector2f vec, Vector2f reference = Vector2f::unitRight);

inline Vector2f lerp(const Vector2f vec1, const Vector2f vec2, const f32 t) {
    return Vector2f(math::lerp(vec1.x, vec2.x, t), math::lerp(vec1.y, vec2.y, t));
}

inline Vector2i lerp(const Vector2i vec1, const Vector2i vec2, const f32 t) {
    return Vector2f(math::lerp(static_cast<f32>(vec1.x), static_cast<f32>(vec2.x), t),
                    math::lerp(static_cast<f32>(vec1.y), static_cast<f32>(vec2.y), t))
        .round();
}

#pragma once

#include <iosfwd>
#include <cassert>

#include "Util/MathUtil.h"
#include "Util/Types.h"

typedef struct Vector2 Vector2;

template <Number T>
struct Vector2T {
    T e[2];

    Vector2T() {
        e[0] = 0;
        e[1] = 0;
    }

    Vector2T(T elem1, T elem2) {
        e[0] = elem1;
        e[1] = elem2;
    }

    Vector2T(const Vector2T<T>& other) {
        e[0] = other.e[0];
        e[1] = other.e[1];
    }

    static inline Vector2T<T> unitUp = {0, 1};
    static inline Vector2T<T> unitDown = {0, -1};
    static inline Vector2T<T> unitLeft{-1, 0};
    static inline Vector2T<T> unitRight{1, 0};
    static inline Vector2T<T> zero{0, 0};

    Vector2T<T>& operator=(const Vector2T<T>& other) {
        if (this != &other) {
            e[0] = other.e[0];
            e[1] = other.e[1];
        }
        return *this;
    }
    inline Vector2T<T> operator+(const Vector2T<T> other) const { return Vector2T<T>(e[0] + other.e[0], e[1] + other.e[1]); }
    inline Vector2T<T> operator+=(const Vector2T<T> other) {
        e[0] += other.e[0];
        e[1] += other.e[1];
        return *this;
    }
    inline Vector2T<T> operator-(const Vector2T<T> other) const { return Vector2T<T>(e[0] - other.e[0], e[1] - other.e[1]); }
    inline Vector2T<T> operator-=(const Vector2T<T> other) {
        e[0] -= other.e[0];
        e[1] -= other.e[1];
        return *this;
    }
    inline Vector2T<T> operator*(const Vector2T<T> other) const { return Vector2T<T>(e[0] * other.e[0], e[1] * other.e[1]); }
    inline Vector2T<T> operator*=(const Vector2T<T> other) {
        e[0] *= other.e[0];
        e[1] *= other.e[1];
        return *this;
    }
    inline Vector2T<T> operator*(const f32 scalar) const { return Vector2T<T>(e[0] * scalar, e[1] * scalar); }
    inline Vector2T<T> operator*=(const f32 scalar) {
        e[0] *= scalar;
        e[1] *= scalar;
        return *this;
    }

    inline Vector2T<T> operator/(const f32 scalar) const {
        assert(scalar != 0 && "Divide By Zero Error");
        return Vector2T<T>(e[0] / scalar, e[1] / scalar);
    }
    inline Vector2T<T> operator/=(const f32 scalar) {
        assert(scalar != 0 && "Divide By Zero Error");
        e[0] /= scalar;
        e[1] /= scalar;
        return *this;
    }

    inline bool operator==(const Vector2T<T> other) const { return e[0] == other.e[0] && e[1] == other.e[1]; }

    inline T x() const { return e[0]; }
    inline T y() const { return e[1]; }

    inline T dot(const Vector2T<T> other) const { return e[0] * other.e[0] + e[1] * other.e[1]; }
    inline T det(const Vector2T<T> other) const { return e[0] * other.e[1] - e[1] * other.e[0]; }
    inline T len() const { return std::sqrt(e[0] * e[0] + e[1] * e[1]); }
    inline Vector2T<T> norm() const {
        assert(!isZero() && "Cannot take norm of zero-length vector");
        T divisor = 1 / len();
        return Vector2T<T>(e[0] * divisor, e[1] * divisor);
    }

    inline Vector2T<T> absolute() const { return Vector2T<T>(abs(e[0]), abs(e[1])); }
    inline bool isZero() const { return e[0] == 0 && e[1] == 0; }
};

template <typename T>
std::ostream& operator<<(std::ostream& out, Vector2T<T> const& self);

typedef Vector2T<f32> Vector2f;
typedef Vector2T<s32> Vector2i;

Vector2f toFloatVec(const Vector2i intVec);
Vector2i toIntVec(const Vector2f floatVec);
Vector2f fromRaylib(Vector2 rlVec);
Vector2i fromRaylibInt(Vector2 rlVec);

inline Vector2f lerp(const Vector2f vec1, const Vector2f vec2, const f32 t) {
    return Vector2f(myLerp(vec1.x(), vec2.x(), t), myLerp(vec1.y(), vec2.y(), t));
}

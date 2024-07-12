#include "Vector.h"

#include <ostream>
#include <raylib.h>

using std::ostream;

template <typename T>
std::ostream& operator<<(std::ostream& out, Vector2T<T> const& self) {
    return out << "(" << self.e[0] << ", " << self.e[1] << ")";
}

Vector2f toFloatVec(const Vector2i intVec) {
    return Vector2f(static_cast<f32>(intVec.x()), static_cast<f32>(intVec.y()));
}

Vector2i toIntVec(const Vector2f floatVec) {
    return Vector2i(static_cast<s32>(floatVec.x()), static_cast<s32>(floatVec.y()));
}

Vector2i toIntVecRounded(const Vector2f floatVec) {
    return Vector2i(std::roundf(floatVec.x()), std::roundf(floatVec.y()));
}

Vector2f fromRaylib(Vector2 rlVec) {
    return {rlVec.x, rlVec.y};
}

Vector2i fromRaylibInt(Vector2 rlVec) {
    return Vector2i(rlVec.x, rlVec.y);
}

Vector2f angleToUnit(f32 angle) {
    f32 radians = angle * DEG2RAD;
    return {std::cos(radians), std::sin(radians)};
}

// DECLARE ALL INSTANTIATIONS OF VECTOR (that i want to print)
template ostream& operator<<(std::ostream& out, Vector2T<s32> const& self);
template ostream& operator<<(std::ostream& out, Vector2T<f32> const& self);

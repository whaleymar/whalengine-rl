#include "Vector.h"

#include <ostream>
#include <raylib.h>

using std::ostream;

// Vector2f::unitUp = {0, 1};
// Vector2f::unitDown = {0, -1};
// Vector2f::unitLeft = {-1, 0};
// Vector2f::unitRight = {1, 0};
// Vector2f::zero = {0,0};

// Vector2i::unitUp = {0, 1};
// Vector2i::unitDown = {0, -1};
// Vector2i::unitLeft = {-1, 0};
// Vector2i::unitRight = {1, 0};
// Vector2i::zero = {0,0};

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

Vector2f fromRaylib(Vector2 rlVec) {
    return {rlVec.x, rlVec.y};
}

Vector2i fromRaylibInt(Vector2 rlVec) {
    return Vector2i(rlVec.x, rlVec.y);
}

// DECLARE ALL INSTANTIATIONS OF VECTOR (that i want to print)
template ostream& operator<<(std::ostream& out, Vector2T<s32> const& self);
template ostream& operator<<(std::ostream& out, Vector2T<f32> const& self);

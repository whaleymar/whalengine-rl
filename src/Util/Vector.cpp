#include "Vector.h"

#include <ostream>
#include <raylib.h>

using std::ostream;

template <typename T>
std::ostream& operator<<(std::ostream& out, Vector2T<T> const& self) {
    return out << "(" << self.x << ", " << self.y << ")";
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

#include "Vector.h"

#include <ostream>
#include <raylib.h>
#include "Util/MathUtil.h"

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

Vector2 toRaylib(Vector2i vec) {
    return Vector2(vec.x, vec.y);
}

Vector2 toRaylib(Vector2f vec) {
    return Vector2(vec.x, vec.y);
}

Vector2f angleToUnit(f32 angle) {
    f32 radians = angle * DEG2RAD;
    return {std::cos(radians), std::sin(radians)};
}

f32 getAngleClockwise(Vector2f vec, Vector2f reference) {
    vec = vec.norm();
    reference = reference.norm();

    f32 dot = vec.dot(reference);
    f32 det = vec.det(reference);
    f32 angleRadians = std::atan2(det, dot);
    f32 angleDegrees = angleRadians * RAD_TO_DEG;

    // angles >180 are negative. clamp between 0 and 360
    if (angleDegrees < 0.0f) {
        return 360.0f + angleDegrees;
    }
    return angleDegrees;
}

f32 getAngle(Vector2f vec, Vector2f reference) {
    vec = vec.norm();
    reference = reference.norm();

    f32 dot = vec.dot(reference);
    f32 det = vec.det(reference);
    f32 angleRadians = -1.0f * std::atan2(det, dot);
    f32 angleDegrees = angleRadians * RAD_TO_DEG;

    // angles >180 are negative. clamp between 0 and 360
    if (angleDegrees < 0.0f) {
        return 360.0f + angleDegrees;
    }
    return angleDegrees;
}

// DECLARE ALL INSTANTIATIONS OF VECTOR (that i want to print)
template ostream& operator<<(std::ostream& out, Vector2T<s32> const& self);
template ostream& operator<<(std::ostream& out, Vector2T<f32> const& self);

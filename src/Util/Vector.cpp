#include "Vector.h"

#include <raylib.h>

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

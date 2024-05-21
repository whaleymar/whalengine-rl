#include "Vector.h"

namespace whal {

Vector2f toFloatVec(const Vector2i intVec) {
    return Vector2f(static_cast<f32>(intVec.x()), static_cast<f32>(intVec.y()));
}

Vector2i toIntVec(const Vector2f floatVec) {
    return Vector2i(static_cast<s32>(floatVec.x()), static_cast<s32>(floatVec.y()));
}

}  // namespace whal

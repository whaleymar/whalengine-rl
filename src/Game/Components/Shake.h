#pragma once

#include "Util/Easing.h"
#include "Util/Types.h"

struct Shake {
    f32 strength = 2.0f;
    f32 duration = 1.0f;
    Ease easeFunc = Ease::InOutSine;

    // managed:
    Vector2f previousOffset = Vector2f::zero;
};

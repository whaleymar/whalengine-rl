#pragma once

#include "Util/MathUtil.h"
enum class Ease {
    Linear,
    InOutBezier,
    InOutSine,
    InQuad,
    OutQuad,
    InCubic,
    OutCubic,
};

f32 ease(const f32 n1, const f32 n2, f32 t, Ease easeFunc) {
    switch (easeFunc) {
    case Ease::Linear:
        return myLerp(n1, n2, t);
    case Ease::InOutBezier:
        return easeInOutBezier(n1, n2, t);
    case Ease::InOutSine:
        return easeInOutSine(n1, n2, t);
    case Ease::InQuad:
        return easeInQuad(n1, n2, t);
    case Ease::OutQuad:
        return easeOutQuad(n1, n2, t);
    case Ease::InCubic:
        return easeInCubic(n1, n2, t);
    case Ease::OutCubic:
        return easeOutCubic(n1, n2, t);
    }
}

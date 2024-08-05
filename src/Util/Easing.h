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

inline f32 getEaseProgress(f32 t, Ease easeFunc) {
    switch (easeFunc) {
    case Ease::Linear:
        return t;
    case Ease::InOutBezier:
        return easeInOutBezier(t);
    case Ease::InOutSine:
        return easeInOutSine(t);
    case Ease::InQuad:
        return easeInQuad(t);
    case Ease::OutQuad:
        return easeOutQuad(t);
    case Ease::InCubic:
        return easeInCubic(t);
    case Ease::OutCubic:
        return easeOutCubic(t);
    }
}

inline f32 ease(const f32 n1, const f32 n2, f32 t, Ease easeFunc) {
    return myLerp(n1, n2, getEaseProgress(t, easeFunc));
}

inline Vector2f ease(const Vector2f n1, const Vector2f n2, f32 t, Ease easeFunc) {
    return lerp(n1, n2, getEaseProgress(t, easeFunc));
}

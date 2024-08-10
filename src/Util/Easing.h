#pragma once

#include "Components/Draw.h"
#include "Util/MathUtil.h"
#include "Util/Vector.h"

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

inline s32 ease(const s32 n1, const s32 n2, f32 t, Ease easeFunc) {
    return std::round(myLerp(static_cast<f32>(n1), static_cast<f32>(n2), getEaseProgress(t, easeFunc)));
}

inline Vector2i ease(const Vector2i n1, Vector2i n2, f32 t, Ease easeFunc) {
    return lerp(n1, n2, getEaseProgress(t, easeFunc));
}

inline Color ease(Color c1, Color c2, f32 t, Ease easeFunc) {
    return whal::Colors::lerp(c1, c2, getEaseProgress(t, easeFunc));
}

// TODO color, then easing

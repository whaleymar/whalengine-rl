#pragma once

#include "Util/Color.h"
#include "Util/Vector.h"

enum class Ease {
    Linear,
    InOutBezier,
    InSine,
    OutSine,
    InOutSine,
    OutInSine,
    InQuad,
    OutQuad,
    InOutQuad,
    OutInQuad,
    InCubic,
    OutCubic,
    InOutCubic,
    OutInCubic,
    InQuart,
    OutQuart,
    InOutQuart,
    OutInQuart,
    InQuint,
    OutQuint,
    InOutQuint,
    OutInQuint,
    InBounce,
    OutBounce,
    InOutBounce,
    OutInBounce,
    InElastic,
    OutElastic,
    InOutElastic,
    OutInElastic,
    InBack,
    OutBack,
    InOutBack,
    OutInBack,
    InSpring,
    OutSpring,
    InOutSpring,
    OutInSpring,
};

f32 getEaseProgress(f32 t, Ease easeFunc);

inline f32 ease(const f32 n1, const f32 n2, f32 t, Ease easeFunc) {
    return math::lerp(n1, n2, getEaseProgress(t, easeFunc));
}

inline Vector2f ease(const Vector2f n1, const Vector2f n2, f32 t, Ease easeFunc) {
    return lerp(n1, n2, getEaseProgress(t, easeFunc));
}

inline s32 ease(const s32 n1, const s32 n2, f32 t, Ease easeFunc) {
    return std::round(math::lerp(static_cast<f32>(n1), static_cast<f32>(n2), getEaseProgress(t, easeFunc)));
}

inline Vector2i ease(const Vector2i n1, Vector2i n2, f32 t, Ease easeFunc) {
    return lerp(n1, n2, getEaseProgress(t, easeFunc));
}

inline Color ease(Color c1, Color c2, f32 t, Ease easeFunc) {
    return whal::Colors::lerp(c1, c2, getEaseProgress(t, easeFunc));
}

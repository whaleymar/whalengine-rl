#pragma once

#include "Util/Types.h"

namespace whal {

enum class Depth : u8 {
    BackgroundStatic,
    BackgroundFar,
    BackgroundMid,
    BackgroundNear,
    Level,
    Player,
    Foreground3,
    Foreground2,
    Foreground1,
    UIFar,
    UIClose,
    Debug,
    BehindPlayer
};

inline constexpr f32 depthToFloat(Depth depth) {
    switch (depth) {
    case Depth::BackgroundStatic:
        return 0.0;
    case Depth::BackgroundFar:
        return 0.1;
    case Depth::BackgroundMid:
        return 0.2;
    case Depth::BackgroundNear:
        return 0.3;
    case Depth::Level:
        return 0.4;
    case Depth::BehindPlayer:
        return 0.45;
    case Depth::Player:
        return 0.5;
    case Depth::Foreground3:
        return 0.6;
    case Depth::Foreground2:
        return 0.7;
    case Depth::Foreground1:
        return 0.8;
    case Depth::UIFar:
        return 0.83;
    case Depth::UIClose:
        return 0.86;
    case Depth::Debug:
        return 0.9;
    }
}

}  // namespace whal

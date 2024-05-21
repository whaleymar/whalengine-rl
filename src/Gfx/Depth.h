#pragma once

#include "Util/Types.h"

namespace whal {

// TODO needs better naming so I know what uses parallax
enum class Depth { Background3, Background2, Background1, Level, Player, Foreground3, Foreground2, Foreground1, Debug };

inline constexpr f32 depthToFloat(Depth depth) {
    switch (depth) {
    case Depth::Background3:
        return 0.0;
    case Depth::Background2:
        return 0.1;
    case Depth::Background1:
        return 0.2;
    case Depth::Level:
        return 0.3;
    case Depth::Player:
        return 0.4;
    case Depth::Foreground3:
        return 0.5;
    case Depth::Foreground2:
        return 0.6;
    case Depth::Foreground1:
        return 0.7;
    case Depth::Debug:
        return 0.9;
    }
}

}  // namespace whal

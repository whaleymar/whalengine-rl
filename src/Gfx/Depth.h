#pragma once

#include "Util/Types.h"

namespace whal {

// These should be ordered correctly, since they're passed as raw data to OpenGL, not floats
enum class Depth : u8 {
    BackgroundStatic,
    BackgroundFar,
    BackgroundMid,
    BackgroundNear,
    Level,
    BehindPlayer,
    Player,
    Foreground3,
    Foreground2,
    Foreground1,
    UIFar,
    UIClose,
    Debug,
};

}  // namespace whal

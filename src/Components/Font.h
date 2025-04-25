#pragma once

#include "Settings.h"
#include "Util/Types.h"
#include "raylib.h"
namespace whal {

// TODO put menu font here?
struct Font {
    rl::Font normal;
};

inline s32 getFontSize() {
    return 40 * VIRTUAL_SCREEN_RATIO / 4.0f;
}

}  // namespace whal

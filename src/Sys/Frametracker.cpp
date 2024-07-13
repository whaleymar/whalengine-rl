#include "Frametracker.h"

namespace whal {

constexpr s32 MAX_FRAME_COUNT = 60;

void Frametracker::update() {
    mFrame++;
    if (mFrame != MAX_FRAME_COUNT) {
        return;
    }
    mFrame = 0;
}

}  // namespace whal

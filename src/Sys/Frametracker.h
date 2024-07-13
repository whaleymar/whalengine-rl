#pragma once

#include "Util/Types.h"

namespace whal {

struct System;

class Frametracker {
public:
    friend System;

    s32 getFrame() const { return mFrame; }
    void update();

private:
    Frametracker() = default;
    Frametracker(const Frametracker&) = delete;
    void operator=(const Frametracker&) = delete;

    s32 mFrame = 0;
};

}  // namespace whal

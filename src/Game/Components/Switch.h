#pragma once

#include "Util/Types.h"

namespace whal::ecs {
using EntityID = u32;
}

struct Switch {
    whal::ecs::EntityID target;
};

struct SwitchGate {
    s32 numKeys = 1;
    bool isPersistent = true;  // if false, resets on death (unused)
};

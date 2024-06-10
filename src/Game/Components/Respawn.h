#pragma once

#include "Util/Types.h"

namespace whal {
struct Sprite;
}

// creates event flow when entity dies
// order is 1) onDeath, 2) wait 3) respawnCallback, 4) onRespawn
// null fxn ptrs are fine
struct Respawn {
    using RespawnCallback = void (*)(whal::Sprite);
    using Callback = void (*)();

    f32 waitTime = 0;
    RespawnCallback respawnCallback = nullptr;
    Callback onDeath = nullptr;
    Callback onRespawn = nullptr;
};

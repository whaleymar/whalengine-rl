#pragma once

#include "Util/Types.h"

namespace whal {
struct Transform2D;
}  // namespace whal

// creates event flow when entity dies
// order is 1) onDeath, 2) wait 3) spawn function, 4) onRespawn
// null fxn ptrs are fine
struct Respawn {
    using Spawner = void (*)(whal::Transform2D);
    using Callback = void (*)();

    f32 waitTime = 0;
    Spawner spawnFunction = nullptr;
    Vector2i spawnPosition;
    Callback onDeath = nullptr;
    Callback onRespawn = nullptr;
};

struct IUseCheckpoints {};

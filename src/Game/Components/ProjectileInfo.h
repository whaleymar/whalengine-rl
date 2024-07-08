#pragma once

#include "Util/Types.h"

namespace whal::ecs {
using EntityID = u32;
}

struct ProjectileInfo {
    whal::ecs::EntityID shooterEntityID;
    f32 initialLifetime;
    Vector2f pushStrength;
};

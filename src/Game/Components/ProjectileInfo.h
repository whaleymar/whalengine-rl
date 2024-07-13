#pragma once

#include "Util/Types.h"

namespace whal::ecs {
using EntityID = u32;
}

struct ProjectileInfo {
    whal::ecs::EntityID shooterEntityID;
    f32 initialLifetime;
    f32 explosionRadius;
    Vector2f pushStrength;
};

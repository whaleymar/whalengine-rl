#pragma once

#include "Util/Vector.h"
#include "whalECS/src/Expected.h"

namespace whal::ecs {
class Entity;
}

Expected<whal::ecs::Entity> makeProjectile(Vector2i position, Vector2f velocity, f32 lifetimeSeconds, f32 explosionRadius);

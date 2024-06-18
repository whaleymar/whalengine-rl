#pragma once

#include "Util/Vector.h"
#include "whalECS/src/Expected.h"

namespace whal::ecs {
class Entity;
using EntityID = u32;
}  // namespace whal::ecs

Expected<whal::ecs::Entity> makeProjectile(whal::ecs::EntityID parentEntityID, Vector2i position, Vector2f velocity, f32 lifetimeSeconds,
                                           f32 explosionRadius);

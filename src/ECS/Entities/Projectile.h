#pragma once

#include "Util/Vector.h"
#include "whalECS/src/Expected.h"

namespace whal {

namespace ecs {
class Entity;
}

Expected<ecs::Entity> makeProjectile(Vector2i position, Vector2f velocity, f32 lifetimeSeconds, f32 explosionRadius);

}  // namespace whal

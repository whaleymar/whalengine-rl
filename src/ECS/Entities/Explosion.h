#pragma once

#include "Util/Vector.h"
#include "whalECS/src/Expected.h"

namespace whal::ecs {
class Entity;
}

Expected<whal::ecs::Entity> makeExplosionZone(Vector2i center, s32 halflen);

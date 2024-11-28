#pragma once

#include "Gfx/Depth.h"
#include "Util/Types.h"

namespace rl {
typedef struct Color Color;
}

namespace whal {

namespace ecs {
class Entity;
}

struct Transform;
struct Sprite;
enum class WorldMaterial : u8;
enum class Direction : u8;

ecs::Entity createParticle(Transform transform, WorldMaterial material, Depth depth = Depth::Level, f32 lifetimeMultiplier = 1.0);

void particleBurst(Transform transform, Direction direction, WorldMaterial material, s32 count = 1, Depth depth = Depth::Level,
                   f32 lifetimeMultiplier = 1.0, f32 speedMultiplier = 1.0);

}  // namespace whal

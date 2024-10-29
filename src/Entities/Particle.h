#pragma once

#include "Components/Transform.h"
#include "Gfx/Depth.h"
#include "Util/Types.h"
#include "whalECS/src/Expected.h"

typedef struct Color Color;

namespace whal {

namespace ecs {
class Entity;
}

struct Transform2D;
struct Sprite;
enum class WorldMaterial : u8;

Expected<ecs::Entity> createParticle(Transform2D transform, WorldMaterial material, Depth depth = Depth::Level, f32 lifetimeMultiplier = 1.0);
Expected<ecs::Entity> createParticleSprite(Transform2D transform, Sprite sprite, f32 lifetime, bool fullRadiance = true);
void particleBurst(Transform2D transform, Direction direction, WorldMaterial material, s32 count = 1, Depth depth = Depth::Level,
                   f32 lifetimeMultiplier = 1.0, f32 speedMultiplier = 1.0);

}  // namespace whal

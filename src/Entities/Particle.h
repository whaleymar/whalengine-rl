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

// convenience functions to create 1x1 texel particles

// TODO all these should be consolidated to the WorldMaterial one
Expected<ecs::Entity> createParticle(Transform2D transform, WorldMaterial material, Depth depth = Depth::Level, f32 lifetimeMultiplier = 1.0);
Expected<ecs::Entity> createParticleLight(Transform2D transform, Color color, f32 lifetime, bool fullRadiance = true, Depth depth = Depth::Level);
Expected<ecs::Entity> createParticleSprite(Transform2D transform, Sprite sprite, f32 lifetime, bool fullRadiance = true, Depth depth = Depth::Level);
void particleBurst(Transform2D transform, Direction direction, WorldMaterial material, s32 count = 1, Depth depth = Depth::Level,
                   f32 lifetimeMultiplier = 1.0);

}  // namespace whal

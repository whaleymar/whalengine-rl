#pragma once

#include "Util/Types.h"
#include "whalECS/src/Expected.h"

typedef struct Color Color;

namespace whal {

namespace ecs {
class Entity;
}

struct Transform2D;
struct Sprite;

// convenience functions to create 1x1 texel particles

Expected<ecs::Entity> createParticle(Transform2D transform, Color color, f32 lifetime = 1.0);
Expected<ecs::Entity> createParticleLight(Transform2D transform, Color color, f32 lifetime, bool fullRadiance = true);
Expected<ecs::Entity> createParticleSprite(Transform2D transform, Sprite sprite, f32 lifetime, bool fullRadiance = true);

}  // namespace whal

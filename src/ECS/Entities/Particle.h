#pragma once

#include "whalECS/src/Expected.h"

typedef struct Color Color;

namespace whal {

namespace ecs {
class Entity;
}

struct Transform2D;

// convenience functions to create 1x1 texel particles

Expected<ecs::Entity> createParticle(Transform2D transform, Color color, f32 lifetime = 1.0);
Expected<ecs::Entity> createParticleLight(Transform2D transform, Color color, f32 lifetime);

}  // namespace whal

#pragma once

#include "Gfx/Depth.h"
#include "Util/Types.h"

namespace rl {
typedef struct Color Color;
}

template <typename T>
struct Vector2;

namespace whal {

namespace ecs {
class Entity;
}

struct Transform;
struct Sprite;
enum class WorldMaterial : u8;
enum class Direction : u8;
struct MaterialData;

ecs::Entity createParticle(Vector2<s32> worldPosition, WorldMaterial material, Depth depth = Depth::Level, f32 lifetimeMultiplier = 1.0);
ecs::Entity createParticle(Vector2<s32> worldPosition, const MaterialData& material, Depth depth = Depth::Level, f32 lifetimeMultiplier = 1.0);

void particleBurst(Transform transform, Direction direction, WorldMaterial material, s32 count = 1, Depth depth = Depth::Level,
                   f32 lifetimeMultiplier = 1.0, f32 minSpeed = 5.0, f32 maxSpeed = 20.0);
void particleBurst(Transform transform, Direction direction, const MaterialData& material, s32 count = 1, Depth depth = Depth::Level,
                   f32 lifetimeMultiplier = 1.0, f32 minSpeed = 5.0, f32 maxSpeed = 20.0);

}  // namespace whal

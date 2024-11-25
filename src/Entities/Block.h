#pragma once

#include "Physics/Material.h"

namespace whal {

namespace ecs {
class Entity;
}

struct Transform;
struct DrawRect;
struct Sprite;

ecs::Entity createBlock(Transform transform);
ecs::Entity createBlock(Transform transform, DrawRect draw);
ecs::Entity createBlock(Transform transform, Sprite sprite, WorldMaterial material = WorldMaterial::None);
ecs::Entity createDecal(Transform transform, Sprite sprite, bool activate = true);

}  // namespace whal

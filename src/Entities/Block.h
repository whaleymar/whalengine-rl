#pragma once

#include "Physics/Material.h"
#include "whalECS/src/Expected.h"

namespace whal {

namespace ecs {
class Entity;
}

struct Transform;
struct DrawRect;
struct Sprite;

Expected<ecs::Entity> createBlock(Transform transform);
Expected<ecs::Entity> createBlock(Transform transform, DrawRect draw);
Expected<ecs::Entity> createBlock(Transform transform, Sprite sprite, WorldMaterial material = WorldMaterial::None);
Expected<ecs::Entity> createDecal(Transform transform, Sprite sprite, bool activate = true);

}  // namespace whal

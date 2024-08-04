#pragma once

#include "Physics/Material.h"
#include "whalECS/src/Expected.h"

namespace whal {

namespace ecs {
class Entity;
}

struct Transform2D;
struct DrawRect;
struct Sprite;

Expected<ecs::Entity> createBlock(Transform2D transform);
Expected<ecs::Entity> createBlock(Transform2D transform, DrawRect draw);
Expected<ecs::Entity> createBlock(Transform2D transform, Sprite sprite, WorldMaterial material = WorldMaterial::None);
Expected<ecs::Entity> createDecal(Transform2D transform, Sprite sprite, bool activate = true);

}  // namespace whal

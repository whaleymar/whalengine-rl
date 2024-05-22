#pragma once

#include "Physics/Material.h"
#include "whalECS/src/Expected.h"

namespace whal {

namespace ecs {
class Entity;
}

struct Transform2D;
struct Draw;
struct Sprite;

Expected<ecs::Entity> createBlock(Transform2D transform);
Expected<ecs::Entity> createBlock(Transform2D transform, Draw draw);
Expected<ecs::Entity> createBlock(Transform2D transform, Sprite sprite, WorldMaterial material = WorldMaterial::None);
Expected<ecs::Entity> createDecal(Transform2D transform, Sprite sprite);

}  // namespace whal

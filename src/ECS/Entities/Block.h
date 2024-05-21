#pragma once

#include "Physics/Material.h"
#include "whalECS/src/Expected.h"

namespace whal {

namespace ecs {
class Entity;
}

struct Transform;
struct Draw;
struct Sprite;

Expected<ecs::Entity> createBlock(Transform transform);
Expected<ecs::Entity> createBlock(Transform transform, Draw draw);
Expected<ecs::Entity> createBlock(Transform transform, Sprite sprite, Material material = Material::None);
Expected<ecs::Entity> createDecal(Transform transform, Sprite sprite);

}  // namespace whal

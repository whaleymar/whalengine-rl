#pragma once

#include "whalECS/src/Expected.h"

namespace whal {

namespace ecs {
class Entity;
}
struct Transform2D;
struct Sprite;

Expected<ecs::Entity> createPlayer();

void respawnPlayer(Transform2D transform, Sprite sprite);

}  // namespace whal

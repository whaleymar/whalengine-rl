#pragma once

#include "whalECS/src/Expected.h"

namespace whal {

namespace ecs {
class Entity;
}
struct Sprite;

Expected<ecs::Entity> createPlayer();

// use sprite with pre-constructed vao/vbo created on main thread
void respawnPlayer(Sprite sprite);

}  // namespace whal

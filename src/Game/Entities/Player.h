#pragma once

#include "whalECS/src/Expected.h"

namespace whal {
namespace ecs {
class Entity;
}
struct Transform2D;
}  // namespace whal

Expected<whal::ecs::Entity> createPlayer();

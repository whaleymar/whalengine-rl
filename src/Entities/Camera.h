#pragma once

#include "whalECS/src/Expected.h"

namespace whal {

struct RailsControl;

namespace ecs {
class Entity;
}

struct Transform2D;

Expected<ecs::Entity> createCamera(Transform2D transform);

}  // namespace whal

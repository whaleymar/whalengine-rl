#pragma once

#include "whalECS/src/Expected.h"

namespace whal {

namespace ecs {
class Entity;
}

struct Transform;

Expected<ecs::Entity> createCamera(Transform transform);

}  // namespace whal

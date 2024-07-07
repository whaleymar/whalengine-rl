#pragma once

#include "Util/Vector.h"
#include "whalECS/src/Expected.h"

namespace whal {

struct RailsControl;

namespace ecs {
class Entity;
}

struct Transform2D;

Expected<ecs::Entity> createCamera(Transform2D transform);
RailsControl createCameraMoveController(Vector2i currentPosition, Vector2i nextPosition);

}  // namespace whal

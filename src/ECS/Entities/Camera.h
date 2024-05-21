#pragma once

#include "Util/Vector.h"
#include "whalECS/src/Expected.h"

namespace whal {

struct RailsControl;

namespace ecs {
class Entity;
}

Expected<ecs::Entity> createCamera(ecs::Entity target);
RailsControl createCameraMoveController(Vector2i currentPosition, Vector2i nextPosition);

}  // namespace whal

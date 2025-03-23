#pragma once

#include "Util/Vector.h"

namespace whal {

namespace ecs {
class Entity;
}

ecs::Entity getCamera();
Vector2i getCameraPosition();
Vector2f getCameraPositionPrecise();
void setCameraPosition(Vector2i pos);
void setCameraTarget(ecs::Entity target);

}  // namespace whal

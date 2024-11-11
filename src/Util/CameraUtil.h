#pragma once

#include "CorradeOptional.h"
#include "Util/Vector.h"

namespace whal {

Corrade::Containers::Optional<ecs::Entity> getCamera();
Vector2i getCameraPosition();
Vector2f getCameraPositionPrecise();
void setCameraPosition(Vector2i pos);
void setCameraTarget(ecs::Entity target);

}  // namespace whal

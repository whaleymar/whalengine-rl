#pragma once

#include "Util/Vector.h"
namespace whal {

Vector2i screenToWorldCoords(Vector2i screenCoords);
Vector2f worldToUVcoords(Vector2f worldCoords);

}  // namespace whal

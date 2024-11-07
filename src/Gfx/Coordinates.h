#pragma once

#include "Util/Vector.h"
namespace whal {

Vector2i screenToWorldCoords(Vector2i screenCoords);
Vector2i worldToTileCoords(Vector2i worldCoords);
Vector2i tileToWorldCoords(Vector2i tileCoords);
Vector2i clampToTile(Vector2i worldCoord);
Vector2f worldToUVcoords(Vector2f worldCoords);

}  // namespace whal

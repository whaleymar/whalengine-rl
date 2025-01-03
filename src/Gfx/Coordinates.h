#pragma once

#include "Util/Vector.h"
namespace whal {

Vector2f screenToWorldCoords(Vector2i screenCoords);
Vector2i worldToScreenCoords(Vector2f worldCoords, Vector2f cameraPosition);
Vector2i worldToTileCoords(Vector2i worldCoords);
Vector2i tileToWorldCoords(Vector2i tileCoords);
Vector2i clampToTile(Vector2i worldCoord);
Vector2f worldToUVcoords(Vector2f worldCoords);

}  // namespace whal

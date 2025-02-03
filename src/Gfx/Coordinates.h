#pragma once

#include "Util/Vector.h"

enum class ScreenResolution;

namespace whal {

Vector2f screenToWorldCoords(Vector2i screenCoords, ScreenResolution resolution);
Vector2i worldToScreenCoords(Vector2f worldCoords, Vector2f cameraPosition, ScreenResolution resolution);
Vector2i worldToTileCoords(Vector2i worldCoords);
Vector2i tileToWorldCoords(Vector2i tileCoords);
Vector2i clampToTile(Vector2i worldCoord);
Vector2f worldToUVcoords(Vector2f worldCoords);
Vector2f worldToRenderCoords(Vector2f worldCoords);  // convert world position to scaled render position (use this when in raylib's 2D mode)

}  // namespace whal

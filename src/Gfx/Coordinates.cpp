#include "Coordinates.h"

#include "Settings.h"
#include "Util/Vector.h"

#include "Systems/TagTrackers.h"

namespace whal {

Vector2i screenToWorldCoords(Vector2i screenCoords) {
    const auto cameraPos = getCameraPosition();

    constexpr f32 invVirtualScreenRatio = 1.0f / VIRTUAL_SCREEN_RATIO;
    const auto middleOffset = Vector2i(WINDOW_WIDTH_GAME / 2, WINDOW_HEIGHT_GAME / 2);

    const Vector2i yAtTop = Vector2i(screenCoords.x, (static_cast<s32>(WINDOW_HEIGHT_RENDER) - screenCoords.y)) * invVirtualScreenRatio;
    return yAtTop + cameraPos - middleOffset;
}

// (0,0) at BOTTOM LEFT for this function
Vector2f worldToUVcoords(Vector2f worldCoords) {
    const Vector2f resolutonRecip = Vector2f(1.0 / FWINDOW_WIDTH_GAME, 1.0 / FWINDOW_HEIGHT_GAME);
    const Vector2f screenHalf = Vector2f(FWINDOW_WIDTH_GAME, FWINDOW_HEIGHT_GAME) * 0.5;
    const Vector2f cameraPos = getCameraPositionPrecise();
    return (Vector2f(worldCoords.x - cameraPos.x, worldCoords.y - cameraPos.y) + screenHalf) * resolutonRecip;
}

}  // namespace whal

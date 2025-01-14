#include "Coordinates.h"

#include "Settings.h"
#include "Util/CameraUtil.h"
#include "Util/Vector.h"

namespace whal {

Vector2f screenToWorldCoords(Vector2i screenCoords) {
    const auto cameraPos = getCameraPositionPrecise();

    const f32 invVirtualScreenRatio = 1.0f / VIRTUAL_SCREEN_RATIO;
    const auto middleOffset = Vector2f(WINDOW_WIDTH_GAME / 2, WINDOW_HEIGHT_GAME / 2);

    const Vector2f yAtTop = Vector2f(screenCoords.x, (WINDOW_HEIGHT_RENDER - screenCoords.y)) * invVirtualScreenRatio;
    return yAtTop + cameraPos - middleOffset;
}

Vector2i worldToScreenCoords(Vector2f worldCoords, Vector2f cameraPosition, bool useGameResolution) {
    const Vector2f screenPositionUnscaled(worldCoords.x - cameraPosition.x, worldCoords.y - cameraPosition.y);
    if (!useGameResolution) {
        const Vector2f screenHalf = Vector2f(FWINDOW_WIDTH_RENDER, FWINDOW_HEIGHT_RENDER) * 0.5;
        return (screenPositionUnscaled * VIRTUAL_SCREEN_RATIO + screenHalf).round();

    } else {
        const Vector2f screenHalf = Vector2f(FWINDOW_WIDTH_GAME, FWINDOW_HEIGHT_GAME) * 0.5;
        return (screenPositionUnscaled + screenHalf).round();
    }
}

Vector2i worldToTileCoords(Vector2i worldCoords) {
    return ((worldCoords + Vector2i(PIXELS_PER_TILE / 2, -PIXELS_PER_TILE + 1)) / PIXELS_PER_TILE);
}

Vector2i tileToWorldCoords(Vector2i tileCoords) {
    return tileCoords * PIXELS_PER_TILE;
}

Vector2i clampToTile(Vector2i worldCoord) {
    return tileToWorldCoords(worldToTileCoords(worldCoord));
}

// (0,0) at BOTTOM LEFT for this function
Vector2f worldToUVcoords(Vector2f worldCoords) {
    const Vector2f resolutonRecip = Vector2f(1.0 / FWINDOW_WIDTH_GAME, 1.0 / FWINDOW_HEIGHT_GAME);
    const Vector2f screenHalf = Vector2f(FWINDOW_WIDTH_GAME, FWINDOW_HEIGHT_GAME) * 0.5;
    const Vector2f cameraPos = getCameraPositionPrecise();
    return (Vector2f(worldCoords.x - cameraPos.x, worldCoords.y - cameraPos.y) + screenHalf) * resolutonRecip;
    // return (Vector2f(worldCoords.x, -worldCoords.y)) * resolutonRecip;
}

Vector2f worldToRenderCoords(Vector2f worldCoords) {
    Vector2f screenPositionUnscaled(worldCoords.x, -worldCoords.y);
    return screenPositionUnscaled * VIRTUAL_SCREEN_RATIO;
}

}  // namespace whal

#include "Coordinates.h"

#include "Settings.h"
#include "Util/CameraUtil.h"
#include "Util/Vector.h"

namespace whal {

Vector2f screenToWorldCoords(Vector2i screenCoords, ScreenResolution resolution) {
    const auto cameraPos = getCameraPositionPrecise();
    const auto middleOffset = Vector2f(WINDOW_WIDTH_GAME / 2, WINDOW_HEIGHT_GAME / 2);

    if (resolution == ScreenResolution::OS || resolution == ScreenResolution::Stretched || resolution == ScreenResolution::Dock) {
        const f32 invVirtualScreenRatio = 1.0f / VIRTUAL_SCREEN_RATIO_STRETCH;

        const Vector2f yAtTop = Vector2f(screenCoords.x, (WINDOW_HEIGHT_STRETCH - screenCoords.y)) * invVirtualScreenRatio;
        return yAtTop + cameraPos - middleOffset;

    } else if (resolution == ScreenResolution::Render) {
        const f32 invVirtualScreenRatio = 1.0f / VIRTUAL_SCREEN_RATIO;

        const Vector2f yAtTop = Vector2f(screenCoords.x, (WINDOW_HEIGHT_RENDER - screenCoords.y)) * invVirtualScreenRatio;
        return yAtTop + cameraPos - middleOffset;

    } else {
        // ScreenResolution::Game
        const Vector2f yAtTop = Vector2f(screenCoords.x, (WINDOW_HEIGHT_GAME - screenCoords.y));
        return yAtTop + cameraPos - middleOffset;
    }
}

Vector2i worldToScreenCoords(Vector2f worldCoords, Vector2f cameraPosition, ScreenResolution res) {
    const Vector2f screenPositionUnscaled(worldCoords.x - cameraPosition.x, worldCoords.y - cameraPosition.y);
    switch (res) {
    case ScreenResolution::OS:
    case ScreenResolution::Dock:
    case ScreenResolution::Stretched:
        return (screenPositionUnscaled * VIRTUAL_SCREEN_RATIO_STRETCH + Vector2f(FWINDOW_WIDTH_STRETCH, FWINDOW_HEIGHT_STRETCH) * 0.5).round();
    case ScreenResolution::Render:
        return (screenPositionUnscaled * VIRTUAL_SCREEN_RATIO + Vector2f(FWINDOW_WIDTH_RENDER, FWINDOW_HEIGHT_RENDER) * 0.5).round();
    case ScreenResolution::Game:
        return (screenPositionUnscaled + Vector2f(FWINDOW_WIDTH_GAME, FWINDOW_HEIGHT_GAME) * 0.5).round();
    }
}

Vector2i worldToTileCoords(Vector2i worldCoords) {
    s32 yOffset = worldCoords.y >= 0 ? PIXELS_PER_TILE / 4 + 1 : -PIXELS_PER_TILE / 2;
    return ((worldCoords + Vector2i(PIXELS_PER_TILE / 2, yOffset)) / PIXELS_PER_TILE);
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

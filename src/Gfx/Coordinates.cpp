#include "Coordinates.h"

#include "Settings.h"
#include "Util/Vector.h"

#include "Systems/TagTrackers.h"

namespace whal {

Vector2i screenToWorldCoords(Vector2i screenCoords) {
    auto cameraPos = getCameraPosition();

    f32 multX = static_cast<f32>(WINDOW_WIDTH_PIXELS) / static_cast<f32>(WINDOW_WIDTH_ACTUAL);
    f32 multY = static_cast<f32>(WINDOW_HEIGHT_PIXELS) / static_cast<f32>(WINDOW_HEIGHT_ACTUAL);
    auto middleOffset = Vector2i(WINDOW_WIDTH_PIXELS / 2, WINDOW_HEIGHT_PIXELS / 2);

    Vector2i yAtTop = Vector2i(screenCoords.x * multX, (static_cast<s32>(WINDOW_HEIGHT_ACTUAL) - screenCoords.y) * multY);
    return yAtTop + cameraPos - middleOffset;
}

// (0,0) at BOTTOM LEFT for this function
Vector2f worldToUVcoords(Vector2f worldCoords) {
    const Vector2f resolutonRecip = Vector2f(1.0 / FWINDOW_WIDTH_PIXELS, 1.0 / FWINDOW_HEIGHT_PIXELS);
    const Vector2f screenHalf = Vector2f(FWINDOW_WIDTH_PIXELS, FWINDOW_HEIGHT_PIXELS) * 0.5;
    const Vector2f cameraPos = getCameraPositionPrecise();
    return (Vector2f(worldCoords.x - cameraPos.x, worldCoords.y - cameraPos.y) + screenHalf) * resolutonRecip;
}

}  // namespace whal

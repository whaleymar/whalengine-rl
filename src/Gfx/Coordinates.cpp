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

}  // namespace whal

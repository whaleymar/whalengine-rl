#include "Common.h"

namespace whal {

RaylibDrawParams getDrawParams(Vector2f position, Vector2f frameSize, Vector2f cameraPosition, Vector2f scale, bool isRotateAboutCenter) {
    Vector2f size = frameSize * scale;
    Vector2f screenPosition(position.x - cameraPosition.x, cameraPosition.y - position.y);

    // rotate about center or transform
    Vector2f origin;
    if (isRotateAboutCenter) {
        origin = size * Vector2f(0.5, 0.5);
        screenPosition.y -= size.y * 0.5;
    } else {
        origin = Vector2f(size.x * 0.5, size.y);
    }

    // Scale everything up
    screenPosition *= VIRTUAL_SCREEN_RATIO;
    screenPosition += Vector2f(FWINDOW_WIDTH_RENDER / 2, FWINDOW_HEIGHT_RENDER / 2);
    size *= VIRTUAL_SCREEN_RATIO;
    origin *= VIRTUAL_SCREEN_RATIO;

    return RaylibDrawParams{
        .rect = Rectangle{screenPosition.x, screenPosition.y, size.x, size.y},
        .origin = Vector2{origin.x, origin.y},
        .position = Vector2{screenPosition.x, screenPosition.y},
    };
}

}  // namespace whal

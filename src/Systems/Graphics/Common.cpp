#include "Common.h"
#include <raylib.h>

#include "Components/Collision.h"
#include "Components/Floating.h"
#include "Components/Transform.h"
#include "Settings.h"

namespace whal::gfx {

PreciseTransform2D getPreciseTrans(ecs::Entity entity) {
    assert(entity.has<Transform2D>());

    PreciseTransform2D pTrans = PreciseTransform2D::fromTrans(entity.get<Transform2D>());
    if (entity.has<PreciseTransform2D>()) {
        pTrans.position = entity.get<PrecisePosition>().position;
    } else if (entity.has<Collider>()) {
        // Make physics movement look smooth even though it's pixel perfect
        pTrans.position += entity.get<Collider>().getRemainder();
    }
    if (entity.has<Floating>()) {
        pTrans.floatHeight = entity.get<Floating>().height;
    }
    return pTrans;
}

PreciseTransform2D getPreciseTrans(ecs::Entity entity, const Transform2D& transform) {
    PreciseTransform2D pTrans = PreciseTransform2D::fromTrans(transform);
    if (entity.has<PreciseTransform2D>()) {
        pTrans.position = entity.get<PrecisePosition>().position;
    } else if (entity.has<Collider>()) {
        // Make physics movement look smooth even though it's pixel perfect
        pTrans.position += entity.get<Collider>().getRemainder();
    }
    if (entity.has<Floating>()) {
        pTrans.floatHeight = entity.get<Floating>().height;
    }
    return pTrans;
}

void clampToPixelGrid(RaylibDrawParams& params) {
    Vector2f positionF = Vector2f(params.position);
    Vector2i position = (positionF / VIRTUAL_SCREEN_RATIO).round() * static_cast<s32>(VIRTUAL_SCREEN_RATIO);

    params.position = position.asRL();
    params.rect.x = params.position.x;
    params.rect.y = params.position.y;
}

RaylibDrawParams getDrawParams(PreciseTransform2D transform, Vector2f frameSize, Vector2f cameraPosition) {
    Vector2f size = frameSize * transform.scale;
    const Vector2f positionF = transform.getRotatedPosition();
    Vector2f screenPosition(positionF.x - cameraPosition.x, cameraPosition.y - positionF.y);

    // rotate about center
    Vector2f origin;
    origin = size * Vector2f(0.5, 0.5);

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

}  // namespace whal::gfx

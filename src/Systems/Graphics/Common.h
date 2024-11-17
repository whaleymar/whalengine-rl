#pragma once

#include <raylib.h>
#include "Components/Transform.h"
#include "CorradeOptional.h"
#include "Physics/Shapes.h"
#include "Util/Vector.h"
#include "whalECS/src/ECS.h"

namespace whal {

class TextureAtlas;

namespace gfx {

struct RaylibDrawParams {
    Rectangle rect;  // includes position
    Vector2 origin;
    Vector2 position;  // for convenience
};

struct RenderContext {
    Vector2f cameraPosition;
    Camera2D camera;
    const TextureAtlas& atlas;
    ecs::Entity cameraEntity;

    // for post processing flags to work right, iRender systems should draw with this color if it's present
    Corrade::Containers::Optional<Color> colorOverride = Corrade::Containers::NullOpt;
};

struct EntityRenderInfo {
    AABB boundingBox;
    PreciseTransform2D preciseTransform;
    ecs::Entity entity;
    const ecs::IRender* piRender;
};

PreciseTransform2D getPreciseTrans(ecs::Entity entity);

// slightly more efficient if caller already has the transform
PreciseTransform2D getPreciseTrans(ecs::Entity entity, const Transform2D& transform);
void clampToPixelGrid(RaylibDrawParams& params);

RaylibDrawParams getDrawParams(PreciseTransform2D transform, Vector2f frameSize, Vector2f cameraPosition);

}  // namespace gfx
}  // namespace whal

#pragma once

#include <raylib.h>
#include "CorradeOptional.h"
#include "Gfx/Depth.h"
#include "Physics/Shapes.h"
#include "Util/Vector.h"
#include "whalECS/src/ECS.h"

namespace whal {

class TextureAtlas;

struct RaylibDrawParams {
    Rectangle rect;  // includes position
    Vector2 origin;
    Vector2 position;  // for convenience
};

struct RenderContext {
    Vector2f cameraPosition;
    Camera2D camera;
    const TextureAtlas& atlas;

    // for post processing flags to work right, iRender systems should draw with this color if it's present
    Corrade::Containers::Optional<Color> colorOverride = Corrade::Containers::NullOpt;
};

struct EntityRenderInfo {
    AABB boundingBox;
    Depth depth;
    ecs::Entity entity;
    const ecs::IRender* piRender;
};

RaylibDrawParams getDrawParams(Vector2f position, Vector2f frameSize, Vector2f cameraPosition, Vector2f scale, bool isRotateAboutCenter);
RaylibDrawParams getDrawParamsNew(PreciseTransform2D transform, Vector2f frameSize, Vector2f cameraPosition);

}  // namespace whal

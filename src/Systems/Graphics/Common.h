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
};

struct ColorBufInfo {
    u8 depth;
    bool isOccluder;
    bool isUI;
};

// TODO split this into 2 structs. One with just the AABB (only needed for culling) and one with PreciseTransform2D (only calculated by Renderer if
// not culled)
//  - difficult thing is that some classes (like DropShadowRenderer) manually alter the PreciseTransform2D calculation... so maybe not
//  - getting rid of the bounding box could still be nice for efficiency?
//    - also don't need IRender implementer to add piRender, renderer can add that to struct w/ PreciseTransform
struct EntityRenderInfo {
    AABB boundingBox;
    PreciseTransform2D preciseTransform;
    ecs::Entity entity;
    const ecs::IRender* piRender;
    ColorBufInfo colorBuf = {};
};

PreciseTransform2D getPreciseTrans(ecs::Entity entity);

// slightly more efficient if caller already has the transform
PreciseTransform2D getPreciseTrans(ecs::Entity entity, const Transform2D& transform);
void clampToPixelGrid(RaylibDrawParams& params);

RaylibDrawParams getDrawParams(PreciseTransform2D transform, Vector2f frameSize, Vector2f cameraPosition);

}  // namespace gfx
}  // namespace whal

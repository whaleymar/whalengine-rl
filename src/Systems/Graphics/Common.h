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
    rl::Rectangle rect;  // includes position
    rl::Vector2 origin;
    rl::Vector2 position;  // for convenience
};

struct RenderContext {
    Vector2f cameraPosition;
    rl::Camera2D camera;
    const TextureAtlas& atlas;
    ecs::Entity cameraEntity;
};

// Stores arbitrary per-pixel information into a separate buffer when drawing.
struct ColorBufInfo {
    u8 depth;
    bool isOccluder;
    bool isUI;

    rl::Vector3 asRL() const;
    static const ColorBufInfo NONE;
};

// TODO split this into 2 structs. One with just the AABB (only needed for culling) and one with PreciseTransform (only calculated by Renderer if
// not culled)
//  - difficult thing is that some classes (like DropShadowRenderer) manually alter the PreciseTransform calculation... so maybe not
//  - getting rid of the bounding box could still be nice for efficiency?
//    - also don't need IRender implementer to add piRender, renderer can add that to struct w/ PreciseTransform
struct EntityRenderInfo {
    AABB boundingBox;
    PreciseTransform preciseTransform;
    ecs::Entity entity;
    const ecs::IRender* piRender;
    ColorBufInfo colorBuf = {};
};

PreciseTransform getPreciseTrans(ecs::Entity entity);

// slightly more efficient if caller already has the transform
PreciseTransform getPreciseTrans(ecs::Entity entity, const Transform& transform);
void clampToPixelGrid(RaylibDrawParams& params);

RaylibDrawParams getDrawParams(PreciseTransform transform, Vector2f frameSize, Vector2f cameraPosition);

}  // namespace gfx
}  // namespace whal

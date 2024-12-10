#pragma once

#include <raylib.h>
#include "Components/Transform.h"
#include "CorradeOptional.h"
#include "Physics/Shapes.h"
#include "Util/Vector.h"
#include "whalECS/src/ECS.h"

namespace whal {

class Renderer;
class TextureAtlas;
struct Sprite;

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
struct DrawMetaData {
    u8 depth;
    bool isOccluder;
    bool isUI;

    rl::Vector3 asRL() const;
    rl::Vector3 asRL(const Sprite& sprite, Vector2f textureDims) const;
    static const DrawMetaData NONE;
};

struct EntityRenderInfo {
    f32 bottom;
    PreciseTransform preciseTransform;
    const ecs::IRender* piRender;
    ecs::Entity entity;
    DrawMetaData colorBuf = {};
};

struct EntityRenderLoc {
    AABB boundingBox;
    PreciseTransform preciseTransform;
    ecs::Entity entity;
};

class RenderQueue {
    friend Renderer;

public:
    void push_back(const EntityRenderLoc& thing);
    void setViewBox(const AABB& viewBox) { mCameraViewBox = viewBox; }
    void setActiveRenderer(ecs::IRender* pIRender) { mpIRender = pIRender; }
    void clear() {
        mNormalQueue.clear();
        mUIQueue.clear();
    }

private:
    std::vector<EntityRenderInfo> mNormalQueue;
    std::vector<EntityRenderInfo> mUIQueue;  // UI separate so it's not affected by lighting

    // draw state:
    ecs::IRender* mpIRender;
    AABB mCameraViewBox;
};

PreciseTransform getPreciseTrans(ecs::Entity entity);

// slightly more efficient if caller already has the transform
PreciseTransform getPreciseTrans(ecs::Entity entity, const Transform& transform);
void clampToPixelGrid(RaylibDrawParams& params);

RaylibDrawParams getDrawParams(PreciseTransform transform, Vector2f frameSize, Vector2f cameraPosition);

}  // namespace gfx
}  // namespace whal

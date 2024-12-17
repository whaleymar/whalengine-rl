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
    Transform preciseTransform;
    const ecs::IRender* piRender;
    ecs::Entity entity;
    DrawMetaData colorBuf = {};
    s32 internal;
};

struct EntityPreRenderInfo {
    enum class IsOccluder {
        Yes,
        No,
        Unchecked,
    };

    AABB boundingBox;
    Transform preciseTransform;
    ecs::Entity entity;
    IsOccluder isOccluder = IsOccluder::Unchecked;
    s32 internal = 0;
};

class RenderQueue {
    friend Renderer;

public:
    // returns true if entity was added, false if culled
    bool add(const EntityPreRenderInfo& thing);

    // called by systems that don't want to check if the entity should be culled
    void addPrecalculated(const EntityPreRenderInfo& thing);

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

void clampToPixelGrid(RaylibDrawParams& params);

RaylibDrawParams getDrawParams(const Transform& transform, Vector2f frameSize, Vector2f cameraPosition);

}  // namespace gfx
}  // namespace whal

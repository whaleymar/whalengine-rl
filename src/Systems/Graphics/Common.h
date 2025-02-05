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
    Transform transform;
    f32 bottom;
    ecs::Entity entity;
    const ecs::IRender* piRender;
    DrawMetaData colorBuf = {};
    s32 internal;
    rl::Shader shader;
};

struct EntityPreRenderInfo {
    enum class IsOccluder {
        Yes,
        No,
        Unchecked,
    };

    AABB boundingBox;
    Transform transform;
    ecs::Entity entity;
    IsOccluder isOccluder = IsOccluder::Unchecked;
    s32 internal = 0;
    rl::Shader shader = rl::Shader{.id = 0xffffffff};  // -1 maps to the default sprite shader
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

RaylibDrawParams getDrawParams(const Transform& transform, Vector2f frameSize);

}  // namespace gfx
}  // namespace whal

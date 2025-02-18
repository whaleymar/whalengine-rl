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
    bool isOccludersOnly = false;  // only used when the Renderer needs to tell a render system which "parents" many sprites to draw the occluder ones
                                   // (e.g. TileRenderSystem)
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
    s32 bottom;
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
        MaybeInChildren,  // do this so GI occlusion is checked for non-ysorted TileMapLayers
    };

    AABB boundingBox;
    Transform transform;
    ecs::Entity entity;
    IsOccluder isOccluder = IsOccluder::Unchecked;
    s32 internal = 0;
    rl::Shader shader = rl::Shader{.id = 0xffffffff, .locs = nullptr};  // -1 maps to the default sprite shader
};

class RenderQueue {
    friend Renderer;

public:
    // returns true if entity was added, false if culled
    bool add(const EntityPreRenderInfo& thing);

    // called by systems that don't want to check if the entity should be culled.
    // returns true if `thing` is a light occluder.
    bool addPrecalculated(const EntityPreRenderInfo& thing);

    void setViewBox(const AABB& viewBox) { mCameraViewBox = viewBox; }
    void setGIViewBox(const AABB& viewBox) { mGlobalIlluminationViewBox = viewBox; }
    void setActiveRenderer(ecs::IRender* pIRender) { mpIRender = pIRender; }
    void clear();

private:
    std::vector<EntityRenderInfo> mNormalQueue;
    std::vector<EntityRenderInfo> mUIQueue;              // UI separate so it's not affected by lighting
    std::vector<EntityRenderInfo> mOccluderQueue;        // occluders which aren't visible to the camera
    std::vector<EntityRenderInfo> mOccluderQueueCamera;  // occluders which ARE visible to the camera

    // draw state:
    ecs::IRender* mpIRender;
    AABB mCameraViewBox;
    AABB mGlobalIlluminationViewBox;
};

void clampToPixelGrid(RaylibDrawParams& params);

RaylibDrawParams getDrawParams(const Transform& transform, Vector2f frameSize);

// Returns the screen position of the sector with the given index.
// Sectors are arranged like this:
// 0  1  2
// 7  8  3
// 6  5  4
// Note: sector 8 is equivalent to the camera's viewport.
// Note: GI == Global Illumination.
Vector2f getGISector(s32 sector);

// returns the sector's offset from the center sector in *world* coordinates (012 -> positive Y)
Vector2f getGISectorOffset(s32 sector);

Vector2f getGISectorSize(s32 sector);
Vector2f getGISectorPadding();

}  // namespace gfx
}  // namespace whal

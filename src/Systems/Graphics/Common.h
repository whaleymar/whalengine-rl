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
class Shader;
struct Sprite;

namespace gfx {

struct RaylibDrawParams {
    rl::Rectangle rect;  // includes position
    rl::Vector2 origin;
};

struct RenderContext {
    Vector2f cameraPosition;
    Vector2i cameraViewHalf;
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
    const Transform* transform;
    s32 ysortPosition;
    ecs::Entity entity;
    const ecs::IRender* piRender;
    DrawMetaData colorBuf = {};
    void* internal;
    Shader* shader;
};

struct EntityPreRenderInfo {
    enum class IsOccluder {
        Yes,
        No,
        Unchecked,
        MaybeInChildren,  // do this so GI occlusion is checked for non-ysorted TileMapLayers
    };

    AABB boundingBox;
    const Transform* transform;
    s32 ysortPosition;
    ecs::Entity entity;
    IsOccluder isOccluder = IsOccluder::Unchecked;
    void* internal;
    Shader* shader;
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
    const AABB& getCameraViewBox() const { return mCameraViewBox; }

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
Vector2f getGIPadding();
AABB getGIViewBox(Vector2i cameraPosition, s32 sector);  // gets camera view box for sector

}  // namespace gfx
}  // namespace whal

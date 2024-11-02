#pragma once

#include "Events/Events.h"
#include "Gfx/Pipeline.h"
#include "Sys/System.h"
#include "Systems/Graphics/Common.h"

#include <vector>

namespace whal {

// 1. ECS systems which inherit ecs::IRender are drawn to the TextureID::Staging RenderTexture
// 2. Renders all game objects and lighting onto the TextureID::Main RenderTexture
// 3. Applies post-processing effects
// 4. Renders debug information, like colliders (if applicable)
class Renderer : public IListen<ShaderReloadEvent, true> {
public:
    static Renderer& instance() {
        static Renderer instance_;
        return instance_;
    }

    static void init();
    static void render();
    static void setPostEffects(Pipeline pipeline);
    void onEvent(ShaderReloadEvent) override;

private:
    Renderer();
    Renderer(const Renderer&) = delete;
    void operator=(const Renderer&) = delete;

    void buildRenderQueue(Vector2i cameraPosition);
    void _drawEntities(RenderContext ctx);
    void _drawEffectsMask(RenderContext ctx);
    void _drawOcclusionMask(RenderContext ctx) const;
    void _drawUI(const RenderContext ctx) const;
    void _render();
    void _init() const;

    Camera2D mRaylibCamera;
    Pipeline mPostProcessSteps;
    std::vector<EntityRenderInfo> mRenderQueue;
    std::vector<EntityRenderInfo> mUIRenderQueue;   // UI is stored in a separate queue so it's not affected by lighting
    std::vector<EntityRenderInfo> mOcclusionQueue;  // stored separately for speed, since they need to be drawn twice (color/depth)
    s32 mMainTextureUniform;
    s32 mExposureUniform;
};

}  // namespace whal

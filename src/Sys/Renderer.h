#pragma once

#include "Gfx/Texture.h"
#include "Systems/Graphics/Common.h"

#include <vector>

namespace whal {

struct System;

class Renderer {
public:
    friend System;
    Renderer();
    void init();  // called after OpenGL context established

    // Renders all game objects to the main frame buffer
    // and applies any post processing effects attached to the camera.
    void render();
    MultiTexture getStagingTex() const { return mStagingTexture; }
    RenderTexture getTemporaryRT();

private:
    Renderer(const Renderer&) = delete;
    void operator=(const Renderer&) = delete;

    void buildRenderQueue(Vector2i cameraPosition);
    void drawEntities(gfx::RenderContext ctx);
    void scaleDepthBuffers(gfx::RenderContext ctx) const;
    void drawUI(const gfx::RenderContext ctx) const;

    Camera2D mRaylibCamera;
    std::vector<gfx::EntityRenderInfo> mRenderQueue;
    std::vector<gfx::EntityRenderInfo> mUIRenderQueue;  // UI is stored in a separate queue so it's not affected by lighting
    MultiTexture mStagingTexture;
};

}  // namespace whal

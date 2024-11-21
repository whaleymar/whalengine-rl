#pragma once

#include "Gfx/Texture.h"
#include "Systems/Graphics/Common.h"

#include <vector>

namespace whal {

struct System;

class Renderer {
    struct RTInfo {
        RenderTexture rt;
        TextureFilter filter;  // store filter so it's only changed when necessary
        s32 unusedFrames = 0;
    };

public:
    friend System;
    Renderer();

    // Renders all game objects to the main frame buffer
    // and applies any post processing effects attached to the camera.
    void render();
    MultiTexture getStagingTex() const { return mStagingTexture; }

    // Temporary Render Textures are convenient and stay cached for a few frames. You should manually release them
    // when you're done using them so another process can use it. Otherwise, they will be released at the end of the frame.
    RenderTexture getTemporaryRT(s32 width, s32 height, PixelFormat format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
                                 TextureFilter filter = TEXTURE_FILTER_POINT);

    // Gets a temporary Render Texture with width, height, and format matching the given texture
    RenderTexture getTemporaryRT(Texture reference, TextureFilter filter = TEXTURE_FILTER_POINT);

    // Release a temporary Render Texture. Call in reverse allocation order for best performance.
    void releaseTemporaryRT(RenderTexture rt);

private:
    Renderer(const Renderer&) = delete;
    void operator=(const Renderer&) = delete;
    void init();  // called after OpenGL context established
    void tick();  // called once per frame

    void buildRenderQueue(Vector2i cameraPosition);
    void drawEntities(gfx::RenderContext ctx);
    void scaleDepthBuffers(gfx::RenderContext ctx) const;
    void drawUI(const gfx::RenderContext ctx) const;

    Camera2D mRaylibCamera;
    std::vector<gfx::EntityRenderInfo> mRenderQueue;
    std::vector<gfx::EntityRenderInfo> mUIRenderQueue;  // UI is stored in a separate queue so it's not affected by lighting

    // this will work for low #s, but I might need to use a stack or another data structure in the future
    std::vector<RTInfo> mAvailableRTs;
    std::vector<RTInfo> mUsedRTs;

    MultiTexture mStagingTexture;
};

}  // namespace whal

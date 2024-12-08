#pragma once

#include "Gfx/Texture.h"
#include "Systems/Graphics/Common.h"
#include "raylib.h"

#include <memory>
#include <vector>

namespace whal {

class IShaderProcess;
namespace gfx {

void applyShaders(rl::RenderTexture target, const std::vector<std::shared_ptr<IShaderProcess>>& shaders);

}  // namespace gfx

struct System;

struct UniformVariant {
    enum class UniformType {
        Float,
        Vec2,
        Vec3,
        Vec4,
        Int,
        Vec2i,
        Vec3i,
        Vec4i,
        Texture,
    };

    UniformType tag;

    union {
        f32 uniFloat;
        rl::Vector2 uniVec2;
        rl::Vector3 uniVec3;
        rl::Vector4 uniVec4;
        s32 uniInt;
        s32 uniVec2i[2];
        s32 uniVec3i[3];
        s32 uniVec4i[4];
        rl::Texture uniTex;
    } val;

    s32 uniformLoc;

    void set(rl::Shader handle) const;
};

class Renderer {
    struct RTInfo {
        rl::RenderTexture rt;
        rl::TextureFilter filter;  // store filter so it's only changed when necessary
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
    rl::RenderTexture getTemporaryRT(s32 width, s32 height, rl::PixelFormat format = rl::PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
                                     rl::TextureFilter filter = rl::TEXTURE_FILTER_POINT);

    // Gets a temporary Render Texture with width, height, and format matching the given texture
    rl::RenderTexture getTemporaryRT(rl::Texture reference, rl::TextureFilter filter = rl::TEXTURE_FILTER_POINT);

    // Release a temporary Render Texture. Call in reverse allocation order for best performance.
    void releaseTemporaryRT(rl::RenderTexture rt);

    // Copy src RenderTexture into dst. Optionally use a shader when drawing.
    // If dimensions aren't the same, scaling happens automatically.
    // src and dest should not be the same RenderTexture.
    // If no shader is specified, then the currently active shader will be used.
    void blit(rl::RenderTexture src, rl::RenderTexture dst, rl::Shader shader = {.id = 0, .locs = nullptr});

    // these aren't used, might delete
    void blit(rl::RenderTexture src, rl::RenderTexture dst, std::shared_ptr<IShaderProcess>& shader);
    void blit(rl::RenderTexture src, rl::RenderTexture dst, IShaderProcess& shader);

    // Queue a shader's uniform value to be set the next time `blit` is run with a shader.
    // Alternatively, `setUniforms` can set them manually.
    // Note: queuing is necessary because setUniformValueTexture *MUST* be called after BeginTextureMode
    // Setting other uniforms beforehand works, but isn't best practice. Better to queue them all.
    void queueUniform(UniformVariant uniform);

    // Set the queued uniform values.
    void setUniforms(rl::Shader shader);

private:
    Renderer(const Renderer&) = delete;
    void operator=(const Renderer&) = delete;
    void init();  // called after OpenGL context established
    void tick();  // called once per frame

    void buildRenderQueue(Vector2i cameraPosition);
    void drawEntities(gfx::RenderContext ctx);
    void drawLights(gfx::RenderContext ctx);
    void scaleDepthBuffers(gfx::RenderContext ctx) const;
    void buildDistanceField() const;
    void drawUI(const gfx::RenderContext ctx) const;

    rl::Camera2D mRaylibCamera;
    gfx::RenderQueue mRenderQueue;

    // this will work for low #s, but I might need to use a stack or another data structure in the future
    std::vector<RTInfo> mAvailableRTs;
    std::vector<RTInfo> mUsedRTs;

    MultiTexture mStagingTexture;

    std::vector<UniformVariant> mUniformQueue;
};

}  // namespace whal

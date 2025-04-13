#pragma once

#include "Systems/Graphics/Common.h"
#include "Util/Singleton.h"
#include "raylib.h"

#include <string>
#include <vector>

namespace whal {

class IShaderProcess;
namespace gfx {

void applyShaders(rl::RenderTexture target, const std::vector<IShaderProcess*>& shaders);

}  // namespace gfx

struct System;
struct MultiTexture;

struct UniformVariant {
    enum UniformType {
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
        u32 uniTex;
    } val;

    void set(rl::Shader handle, s32 uniformLoc) const;
};

struct ShaderUniform {
    UniformVariant value;
    s32 loc;
};

class Renderer {
    SINGLETON_CUSTOM(Renderer)
    friend System;
    struct RTInfo {
        rl::RenderTexture rt;
        rl::TextureFilter filter;  // store filter so it's only changed when necessary
        rl::TextureWrap wrap;
        s32 unusedFrames = 0;
    };

    struct GlobalUniformTracker {
        u64 index;
        s32 uniformLoc;
    };

public:
    // Renders all game objects to the main frame buffer
    // and applies any post processing effects attached to the camera.
    void render();
    MultiTexture* getStagingTex() const { return mStagingTexture; }

    // Temporary Render Textures are convenient and stay cached for a few frames. You should manually release them
    // when you're done using them so another process can use it. Otherwise, they will be released at the end of the frame.
    rl::RenderTexture getTemporaryRT(s32 width, s32 height, rl::PixelFormat format = rl::PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
                                     rl::TextureFilter filter = rl::TEXTURE_FILTER_POINT, rl::TextureWrap wrap = rl::TEXTURE_WRAP_CLAMP);

    // Gets a temporary Render Texture with width, height, and format matching the given texture
    rl::RenderTexture getTemporaryRT(rl::Texture reference, rl::TextureFilter filter = rl::TEXTURE_FILTER_POINT,
                                     rl::TextureWrap wrap = rl::TEXTURE_WRAP_CLAMP);

    // Release a temporary Render Texture. Call in reverse allocation order for best performance.
    void releaseTemporaryRT(rl::RenderTexture rt);

    // Copy src RenderTexture into dst. Optionally use a shader when drawing.
    // If dimensions aren't the same, scaling happens automatically.
    // src and dest should not be the same RenderTexture.
    // If no shader is specified, then the currently active shader will be used.
    // If a fixed shader is set (using `fixedShaderMode`), then that shader will be used.
    void blit(rl::RenderTexture src, rl::RenderTexture dst, rl::Shader shader = {.id = 0, .locs = nullptr},
              rl::BlendMode blendMode = rl::BLEND_ALPHA);

    // Queue a shader's uniform value to be set the next time `blit` is run with a shader.
    // Alternatively, `setUniforms` can set them manually.
    // Note: queuing is necessary because setUniformValueTexture *MUST* be called after BeginTextureMode
    // Setting other uniforms beforehand works, but isn't best practice. Better to queue them all.
    void queueUniform(ShaderUniform uniform);

    // Set the queued uniform values.
    void setUniforms(rl::Shader shader);

    // Activates a shader that will not be deactivated until `endFixedShaderMode` is called.
    // if `isPersistUniforms` is true, then calls to `blit` will not clear the uniforms set for that draw call.
    void fixedShaderMode(rl::Shader shader, bool isPersistUniforms = false);
    void endFixedShaderMode();

    gfx::RenderContext getRenderContext(bool useUnstretchedRenderWindow = true) const;

    // updates the {F}WINDOW_{WIDTH/HEIGHT}_{RENDER/OS} variables, as well as the other global variables that depend on them.
    // parentSize is the size of the window the game is rendered to. Is the OS window unless the engine editor is active. Then it is the ImGui window.
    // renderPosition (optional) is the screen coordinates of the render window. If {-1, -1}, then it is automatically calculated to be the center of
    // the parent window
    void updateWindowSizes(Vector2i renderSize, Vector2i parentSize, Vector2i renderPosition = Vector2i(-1, -1));

    void toggleFullscreen();

    /////////////////////
    // Global Uniforms //
    /////////////////////

    // Creates a uniform which is globally accessible by all shaders. This should run before shaders are compiled.
    // If a shader is compiled and it wants an unregistered uniform, the compilation will fail.
    void globalUniformRegister(const std::string& name, UniformVariant initialValue);
    void globalUniformSubscribe(const std::string& name, const Shader& shader);
    void globalUniformBindAll(rl::Shader shader);
    void globalUniformOnShaderUnload(rl::Shader shader);

    // Set the value of a uniform
    void globalUniformSetFloat(const std::string& name, f32 val);
    void globalUniformSetInt(const std::string& name, s32 val);
    void globalUniformSetTexture(const std::string& name, rl::Texture val);
    void globalUniformSetVec2(const std::string& name, Vector2f val);
    void globalUniformSetVec2(const std::string& name, rl::Vector2 val);
    void globalUniformSetVec3(const std::string& name, rl::Vector3 val);
    void globalUniformSetVec4(const std::string& name, rl::Vector4 val);

private:
    bool init();    // called after OpenGL context established. Returns true on error.
    void update();  // called once per frame
    void end();     // called by System::end
    void reset();   // called by System::resetManagers

    void buildRenderQueue(Vector2i cameraPosition, Vector2i cameraViewHalf);
    void drawEntities(gfx::RenderContext ctx);
    void drawRenderQueue(const MultiTexture& target, const gfx::RenderContext& renderContext, const std::vector<gfx::EntityRenderInfo>& queue);
    void drawLights(gfx::RenderContext ctx);
    void scaleDepthBuffers(gfx::RenderContext ctx, rl::Texture updatedSector) const;
    void buildDistanceField() const;
    void drawUI(const gfx::RenderContext ctx) const;

    // updates variables which depend on WINDOW_{WIDTH/HEIGHT}_RENDER
    void cascadeWindowChanges(Vector2i parentSize, Vector2i windowPosition);

    rl::Camera2D mRaylibCamera;
    gfx::RenderQueue mRenderQueue;

    // this will work for low #s, but I might need to use a stack or another data structure in the future
    std::vector<RTInfo> mAvailableRTs;
    std::vector<RTInfo> mUsedRTs;

    MultiTexture* mStagingTexture;
    MultiTexture* mGIOccluderTexture;

    std::vector<ShaderUniform> mUniformQueue;

    rl::Shader mFixedShader;
    Vector2i mPrevWindowSizeBeforeFullscreen;
    Vector2i mPrevWindowPosBeforeFullscreen;

    /////////////////////
    // Global Uniforms //
    /////////////////////

    // maps a shader's ID to a list of tuples containing the uniform location, and an index into mGlobalUniforms
    std::unordered_map<u32, std::vector<GlobalUniformTracker>> mGlobalUniformSubscribers;
    std::unordered_map<std::string, u64> mGlobalUniformNameToIndex;
    std::vector<UniformVariant> mGlobalUniforms;

    bool mIsFixedShaderMode = false;
    bool mIsPersistUniforms = false;
};

}  // namespace whal

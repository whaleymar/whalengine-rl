#include "Tonemap.h"

#include "Gfx/RaylibUtil.h"
#include "Gfx/Texture.h"
#include "raylib.h"

namespace whal {

Tonemap::Tonemap() : BaseShader("", "whalengine/src/Shader/toneMapping.glsl") {}

void Tonemap::process(RenderTexture src, RenderTexture dst) {
    assert(isValid());

    // This can be anything with the render dimensions EXCEPT mStagingTexture.tex, because I want to maintain the other color buffers for debugging
    // TODO getTempRT
    RenderTexture tmpSrc;
    if (src.id == dst.id) {
        tmpSrc = TextureManager::getRenderTexture(TextureID::UpscaledLighting);
        BeginTextureMode(tmpSrc);
        ClearBackground(Colors::CLEAR);
        BeginBlendMode(BLEND_ALPHA_PREMULTIPLY);  // doesn't seem to make a difference
        BeginShaderMode(mShaderHandle);
        gfx::DrawRenderTexture(src, WHITE);
        EndShaderMode();
        EndBlendMode();
        EndTextureMode();
    } else {
        tmpSrc = src;
    }

    // write back to main
    BeginTextureMode(dst);
    ClearBackground(Colors::CLEAR);
    BeginBlendMode(BLEND_ALPHA_PREMULTIPLY);
    gfx::DrawRenderTexture(tmpSrc);
    EndBlendMode();
    EndTextureMode();
}

}  // namespace whal

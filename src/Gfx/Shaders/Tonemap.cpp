#include "Tonemap.h"

#include "Gfx/RaylibUtil.h"
#include "Sys/System.h"
#include "raylib.h"

namespace whal {

Tonemap::Tonemap() : BaseShader("", "whalengine/src/Shader/toneMapping.glsl") {}

void Tonemap::process(RenderTexture src, RenderTexture dst) {
    assert(isValid());

    RenderTexture tmpSrc;
    bool isTemporary = false;
    if (src.id == dst.id) {
        isTemporary = true;
        tmpSrc = Graphics.getTemporaryRT(src.texture);
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

    if (isTemporary) {
        Graphics.releaseTemporaryRT(tmpSrc);
    }
}

}  // namespace whal

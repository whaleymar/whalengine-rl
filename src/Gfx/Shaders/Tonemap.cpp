#include "Tonemap.h"

#include "Gfx/RaylibUtil.h"
#include "raylib.h"

namespace whal {

Tonemap::Tonemap() : BaseShader("", "whalengine/src/Shader/toneMapping.glsl") {}

void Tonemap::process(RenderTexture src, RenderTexture dst) {
    assert(isValid());

    // write back to main
    BeginTextureMode(dst);
    ClearBackground(Colors::CLEAR);
    BeginBlendMode(BLEND_ALPHA_PREMULTIPLY);
    BeginShaderMode(mShaderHandle);
    gfx::DrawRenderTexture(src);
    EndShaderMode();
    EndBlendMode();
    EndTextureMode();
}

}  // namespace whal

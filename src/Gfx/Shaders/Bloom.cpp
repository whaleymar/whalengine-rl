#include "Bloom.h"

#include "Gfx/RaylibUtil.h"
#include "Gfx/Texture.h"
#include "raylib.h"

namespace whal {

Bloom::Bloom() : BaseShader("", "whalengine/src/Shader/threshold.glsl") {
    threshold = 1.5;
}

void Bloom::process(RenderTexture src, RenderTexture dst) {
    assert(isValid());

    // TODO would like some sort of GetTemporaryRenderTex func for this
    RenderTexture halfRes = TextureManager::getRenderTexture(TextureID::HalfResBuf);
    RenderTexture quarterRes = TextureManager::getRenderTexture(TextureID::QuarterResBuf);
    RenderTexture bloomTex = TextureManager::getRenderTexture(TextureID::Bloom);

    BeginTextureMode(bloomTex);
    BeginShaderMode(mShaderHandle);
    setFloat("lum_threshold", threshold);
    gfx::DrawRenderTexture(src);
    EndShaderMode();
    EndTextureMode();

    // downscale
    gfx::ScaleTexture(bloomTex, halfRes);
    gfx::ScaleTexture(halfRes, quarterRes);

    // upscale
    gfx::ScaleTexture(quarterRes, halfRes);
    gfx::ScaleTexture(halfRes, bloomTex);

    // Draw Additively
    BeginTextureMode(dst);
    BeginBlendMode(BLEND_ADDITIVE);
    gfx::DrawRenderTexture(bloomTex);
    EndBlendMode();
    EndTextureMode();
}

}  // namespace whal

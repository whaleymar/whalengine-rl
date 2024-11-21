#include "Bloom.h"

#include "Gfx/RaylibUtil.h"
#include "Gfx/Texture.h"
#include "Sys/System.h"
#include "raylib.h"

namespace whal {

Bloom::Bloom() : BaseShader("", "whalengine/src/Shader/threshold.glsl") {
    threshold = 1.5;
}

void Bloom::process(RenderTexture src, RenderTexture dst) {
    assert(isValid());

    RenderTexture bloomTex = TextureManager::getRenderTexture(TextureID::Bloom);
    RenderTexture halfRes =
        Graphics.getTemporaryRT(src.texture.width / 2, src.texture.height / 2, static_cast<PixelFormat>(src.texture.format), TEXTURE_FILTER_BILINEAR);
    RenderTexture quarterRes =
        Graphics.getTemporaryRT(src.texture.width / 4, src.texture.height / 4, static_cast<PixelFormat>(src.texture.format), TEXTURE_FILTER_BILINEAR);

    BeginTextureMode(bloomTex);
    ClearBackground(Colors::CLEAR);
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

    Graphics.releaseTemporaryRT(quarterRes);
    Graphics.releaseTemporaryRT(halfRes);

    // Draw Additively
    BeginTextureMode(dst);
    ClearBackground(Colors::CLEAR);
    gfx::DrawRenderTexture(src);
    BeginBlendMode(BLEND_ADDITIVE);
    gfx::DrawRenderTexture(bloomTex);
    EndBlendMode();
    EndTextureMode();
}

}  // namespace whal

#include "Bloom.h"

#include "Gfx/RaylibUtil.h"
#include "Gfx/Texture.h"
#include "Sys/System.h"
#include "Util/MathUtil.h"
#include "raylib.h"

namespace whal {

Bloom::Bloom() : mThresh("", "whalengine/src/Shader/threshold.glsl"), mBlur("", "whalengine/src/Shader/BoxBlur.glsl") {
    threshold = 1.2;
    softThreshold = 0.5;
    // intensity = 0.5;
}

void Bloom::process(RenderTexture src, RenderTexture dst) {
    assert(mThresh.isValid() && mBlur.isValid());

    RenderTexture bloomTex = TextureManager::getRenderTexture(TextureID::Bloom);
    auto fmt = static_cast<PixelFormat>(src.texture.format);
    auto filter = TEXTURE_FILTER_BILINEAR;
    RenderTexture halfRes = Graphics.getTemporaryRT(src.texture.width / 2, src.texture.height / 2, fmt, filter);
    RenderTexture quarterRes = Graphics.getTemporaryRT(src.texture.width / 4, src.texture.height / 4, fmt, filter);
    RenderTexture eightRes = Graphics.getTemporaryRT(src.texture.width / 8, src.texture.height / 8, fmt, filter);

    BeginShaderMode(mThresh.get());
    mThresh.setFloat("_Threshold", threshold);
    mThresh.setFloat("_SoftThreshold", softThreshold);
    Graphics.blit(src, bloomTex);
    EndShaderMode();

    // downscale
    BeginShaderMode(mBlur.get());
    Graphics.blit(bloomTex, halfRes);
    Graphics.blit(halfRes, quarterRes);
    Graphics.blit(quarterRes, eightRes);

    // upscale
    Graphics.blit(eightRes, quarterRes);
    Graphics.blit(quarterRes, halfRes);
    Graphics.blit(halfRes, bloomTex);
    EndShaderMode();

    Graphics.releaseTemporaryRT(quarterRes);
    Graphics.releaseTemporaryRT(halfRes);

    // Draw Additively
    // TODO use HDR draw function so intensity modifier isn't clamped to LDR
    const f32 gc = math::gammaToLinear(intensity);
    const Color tint = ColorFromNormalized(Vector4{gc, gc, gc, 1.0f});
    BeginTextureMode(dst);
    ClearBackground(Colors::CLEAR);
    gfx::DrawRenderTexture(src);
    BeginBlendMode(BLEND_ADDITIVE);
    gfx::DrawRenderTexture(bloomTex, tint);
    EndBlendMode();
    EndTextureMode();
}

}  // namespace whal

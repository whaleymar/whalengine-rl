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

void Bloom::process(rl::RenderTexture src, rl::RenderTexture dst) {
    assert(mThresh.isValid() && mBlur.isValid());

    rl::RenderTexture bloomTex = TextureManager::getRenderTexture(TextureID::Bloom);
    auto fmt = static_cast<rl::PixelFormat>(src.texture.format);
    auto filter = rl::TEXTURE_FILTER_BILINEAR;
    rl::RenderTexture halfRes = Graphics.getTemporaryRT(src.texture.width / 2, src.texture.height / 2, fmt, filter);
    rl::RenderTexture quarterRes = Graphics.getTemporaryRT(src.texture.width / 4, src.texture.height / 4, fmt, filter);
    rl::RenderTexture eightRes = Graphics.getTemporaryRT(src.texture.width / 8, src.texture.height / 8, fmt, filter);

    rl::BeginShaderMode(mThresh.get());
    mThresh.setFloat("_Threshold", threshold);
    mThresh.setFloat("_SoftThreshold", softThreshold);
    Graphics.blit(src, bloomTex);
    rl::EndShaderMode();

    // downscale
    rl::BeginShaderMode(mBlur.get());
    Graphics.blit(bloomTex, halfRes);
    Graphics.blit(halfRes, quarterRes);
    Graphics.blit(quarterRes, eightRes);

    // upscale
    Graphics.blit(eightRes, quarterRes);
    Graphics.blit(quarterRes, halfRes);
    Graphics.blit(halfRes, bloomTex);
    rl::EndShaderMode();

    Graphics.releaseTemporaryRT(quarterRes);
    Graphics.releaseTemporaryRT(halfRes);

    // Draw Additively
    // TODO use HDR draw function so intensity modifier isn't clamped to LDR
    const f32 gc = math::gammaToLinear(intensity);
    const rl::Color tint = rl::ColorFromNormalized(rl::Vector4{gc, gc, gc, 1.0f});
    rl::BeginTextureMode(dst);
    rl::ClearBackground(Colors::CLEAR);
    gfx::DrawRenderTexture(src);
    rl::BeginBlendMode(rl::BLEND_ADDITIVE);
    gfx::DrawRenderTexture(bloomTex, tint);
    rl::EndBlendMode();
    rl::EndTextureMode();
}

}  // namespace whal

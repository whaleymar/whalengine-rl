#include "Bloom.h"

#include "Gfx/RaylibUtil.h"
#include "Gfx/ShaderManager.h"
#include "Gfx/Texture.h"
#include "Sys/System.h"
#include "Util/MathUtil.h"
#include "raylib.h"

#ifndef NDEBUG
#include "imgui.h"
#endif

namespace whal {

Bloom::Bloom() {
    threshold = 1.2;
    softThreshold = 0.5;
    // intensity = 0.5;
}

void Bloom::process(rl::RenderTexture src, rl::RenderTexture dst) {
    rl::RenderTexture bloomTex = TextureManager::getRenderTexture(TextureID::Bloom);
    auto fmt = static_cast<rl::PixelFormat>(src.texture.format);
    auto filter = rl::TEXTURE_FILTER_BILINEAR;
    rl::RenderTexture halfRes = Graphics.getTemporaryRT(src.texture.width / 2, src.texture.height / 2, fmt, filter);
    rl::RenderTexture quarterRes = Graphics.getTemporaryRT(src.texture.width / 4, src.texture.height / 4, fmt, filter);
    rl::RenderTexture eightRes = Graphics.getTemporaryRT(src.texture.width / 8, src.texture.height / 8, fmt, filter);

    Shader& shThreshold = ShaderMgr::get("Threshold");
    Shader& shBlur = ShaderMgr::get("BoxBlur");

    shThreshold.setFloat("_Threshold", threshold);
    shThreshold.setFloat("_SoftThreshold", softThreshold);
    Graphics.blit(src, bloomTex, shThreshold.get());

    // downscale
    Graphics.fixedShaderMode(shBlur.get());

    _blurPass(bloomTex, halfRes);
    _blurPass(halfRes, quarterRes);
    _blurPass(quarterRes, eightRes);

    // upscale
    _blurPass(eightRes, quarterRes);
    _blurPass(quarterRes, halfRes);
    _blurPass(halfRes, bloomTex);

    Graphics.endFixedShaderMode();

    Graphics.releaseTemporaryRT(quarterRes);
    Graphics.releaseTemporaryRT(halfRes);

    // Draw Original Scene, then draw Bloom Additively
    const f32 gc = math::gammaToLinear(intensity);
    rl::BeginTextureMode(dst);
    rl::ClearBackground(Colors::ClearRL);
    gfx::DrawRenderTexture(src);
    rl::BeginBlendMode(rl::BLEND_ADDITIVE);
    gfx::DrawRenderTextureHDR(bloomTex, Color{gc, gc, gc, 1.0f});
    rl::EndBlendMode();
    rl::EndTextureMode();
}

#ifndef NDEBUG
void Bloom::drawEditor() {
    ImGui::Begin("Bloom");
    ImGui::SliderFloat("Threshold", &threshold, 0.0f, 10.0f);
    ImGui::SliderFloat("Soft Threshold", &softThreshold, 0.0f, 1.0f);
    ImGui::SliderFloat("Intensity", &intensity, 0.0f, 10.0f);
    ImGui::End();
}
#endif

// this will go somewhere else once I have a framework for built-in uniforms
void Bloom::_blurPass(rl::RenderTexture src, rl::RenderTexture dst) {
// GLSL ES 2.0 doesn't have the textureSize function
#ifdef __EMSCRIPTEN__
    mBlur.setVector2("_TextureSize", Vector2f(src.texture.width, src.texture.height));
#endif
    Graphics.blit(src, dst);
}

}  // namespace whal

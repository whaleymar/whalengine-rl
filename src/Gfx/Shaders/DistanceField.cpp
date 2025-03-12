#include "DistanceField.h"

#include <cmath>
#include "Gfx/RaylibUtil.h"
#include "Gfx/ShaderManager.h"
#include "Sys/System.h"
#include "raylib.h"

#ifndef NDEBUG
#include "imgui.h"
#endif

namespace whal {

static int N_PASSES = 10;

DistanceField::DistanceField() {}

void DistanceField::process(rl::RenderTexture src, rl::RenderTexture dst) {
    rl::PixelFormat format = rl::PIXELFORMAT_UNCOMPRESSED_R16G16B16;
    // rl::PixelFormat format = rl::PIXELFORMAT_UNCOMPRESSED_R8G8B8; // too noisy
    auto tmpOutput1 = Graphics.getTemporaryRT(dst.texture.width, dst.texture.height, format);
    auto tmpOutput2 = Graphics.getTemporaryRT(dst.texture.width, dst.texture.height, format);
    Shader& shUvMask = ShaderMgr::get("UVMask");
    Graphics.blit(src, tmpOutput1, shUvMask.get());
    rl::RenderTexture currentInput = tmpOutput1;
    rl::RenderTexture currentOutput = tmpOutput2;

    // number of passes should be log base 2 of our largest dimension
    const s32 nPasses = std::ceil(std::log2(static_cast<f32>(std::max(src.texture.width, src.texture.height))));
    Vector2f floatResolutionInv(1.0f / static_cast<f32>(src.texture.width), 1.0f / static_cast<f32>(src.texture.height));

    Shader& shJumpFlood = ShaderMgr::get("JumpFloodUV");
    Graphics.fixedShaderMode(shJumpFlood.get());
    for (s32 i = 1; i < nPasses; i++) {
        if (i >= N_PASSES) {
            break;
        }
        const f32 offset = std::pow(2, static_cast<f32>(nPasses - i - 1));
        shJumpFlood.setVector2("_Offset", floatResolutionInv * offset);
        Graphics.blit(currentInput, currentOutput);

        // swap
        // use src as temporary value
        src = currentInput;
        currentInput = currentOutput;
        currentOutput = src;
    }
    Graphics.endFixedShaderMode();

    // convert Jump-Flooded UV field into distance field
    Shader& shDistanceField = ShaderMgr::get("DistanceField");
    Graphics.blit(currentInput, dst, shDistanceField.get());

    // release temporary texture
    Graphics.releaseTemporaryRT(tmpOutput1);
    Graphics.releaseTemporaryRT(tmpOutput2);
}

#ifndef NDEBUG
void DistanceField::drawEditor() {
    ImGui::Begin("DistanceField");
    ImGui::SliderInt("N Flood Passes", &N_PASSES, 1, std::ceil(std::log2(960.0f)));
    ImGui::End();
}
#endif

}  // namespace whal

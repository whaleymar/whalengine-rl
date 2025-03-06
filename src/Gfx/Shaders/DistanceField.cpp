#include "DistanceField.h"

#include <cmath>
#include "Gfx/RaylibUtil.h"
#include "Sys/System.h"
#include "raylib.h"

#ifndef NDEBUG
#include "imgui.h"
#endif

namespace whal {

static int N_PASSES = 10;
static bool SHOW_UV = false;

DistanceField::DistanceField()
    : mUvMask("whalengine/src/Shader/UVMask.glsl"), mJumpFlood("whalengine/src/Shader/JumpFloodUV.glsl"),
      mDistanceField("whalengine/src/Shader/DistanceField.glsl") {}

void DistanceField::process(rl::RenderTexture src, rl::RenderTexture dst) {
    assert(mJumpFlood.isValid() && src.texture.width == dst.texture.width && src.texture.height == dst.texture.height);

    auto tmpOutput = Graphics.getTemporaryRT(dst.texture);
    Graphics.blit(src, tmpOutput, mUvMask.get());
    rl::RenderTexture currentInput = tmpOutput;
    rl::RenderTexture currentOutput = dst;

    // number of passes should be log base 2 of our largest dimension
    const s32 nPasses = std::ceil(std::log2(static_cast<f32>(std::max(src.texture.width, src.texture.height))));
    Vector2f floatResolutionInv(1.0f / static_cast<f32>(src.texture.width), 1.0f / static_cast<f32>(src.texture.height));

    Graphics.fixedShaderMode(mJumpFlood.get());
    for (s32 i = 1; i < nPasses; i++) {
        if (i >= N_PASSES) {
            break;
        }
        const f32 offset = std::pow(2, static_cast<f32>(nPasses - i - 1));
        mJumpFlood.setVector2("_Offset", floatResolutionInv * offset);
        Graphics.blit(currentInput, currentOutput);

        // swap
        // use src as temporary value
        src = currentInput;
        currentInput = currentOutput;
        currentOutput = src;
    }
    Graphics.endFixedShaderMode();

    // make sure latest draw is to tmpOutput
    // (if currentOutput is tmpOutput.id, then we just drew to dst)
    if (currentOutput.id == tmpOutput.id) {
        Graphics.blit(currentInput, currentOutput);
    }

    // convert Jump-Flooded UV field into distance field:
    if (SHOW_UV) {
        Graphics.blit(tmpOutput, dst);
    } else {
        Graphics.blit(tmpOutput, dst, mDistanceField.get());
    }

    // release temporary texture
    Graphics.releaseTemporaryRT(tmpOutput);
}

#ifndef NDEBUG
void DistanceField::drawDebug() {
    ImGui::Begin("DistanceField");
    ImGui::SliderInt("N Flood Passes", &N_PASSES, 1, std::ceil(std::log2(960.0f)));
    ImGui::Checkbox("Show UV", &SHOW_UV);
    ImGui::End();
}
#endif

}  // namespace whal

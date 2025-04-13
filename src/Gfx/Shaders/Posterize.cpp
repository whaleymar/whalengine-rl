#include "Posterize.h"

#include "Gfx/RaylibUtil.h"
#include "Gfx/ShaderManager.h"
#include "Gfx/Texture.h"
#include "Sys/Renderer.h"
#include "Sys/System.h"
#include "raylib.h"

#ifndef NDEBUG
#include "imgui.h"
static bool S_ENABLED = true;
#endif

namespace whal {

void Posterize::process(rl::RenderTexture src, rl::RenderTexture dst) {
#ifndef NDEBUG
    if (!S_ENABLED) {
        Graphics.blit(src, dst, {0, nullptr}, rl::BLEND_ALPHA_PREMULTIPLY);
        return;
    }
#endif
    Shader& posterizeShader = ShaderMgr::get("Quantize");
    auto paletteTex = TextureManager::getTexture(TEXNAME_PALETTE);
    posterizeShader.setTexture("_Palette", paletteTex);
    posterizeShader.setVector2("_PaletteTexSize", rl::Vector2(paletteTex.width, paletteTex.height));
    Graphics.blit(src, dst, posterizeShader.get(), rl::BLEND_ALPHA_PREMULTIPLY);
}

#ifndef NDEBUG
void Posterize::drawEditor() {
    ImGui::Begin("Posterization");
    ImGui::Checkbox("Active", &S_ENABLED);
    ImGui::End();
}
#endif

}  // namespace whal

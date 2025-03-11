#include "Posterize.h"

#include "Gfx/RaylibUtil.h"
#include "Gfx/ShaderManager.h"
#include "Gfx/Texture.h"
#include "Settings.h"
#include "Sys/System.h"
#include "Util/Print.h"
#include "raylib.h"

#ifndef NDEBUG
#include "imgui.h"
static bool S_ENABLED = true;
#endif

namespace whal {

Posterize::Posterize() {
    auto err = TextureManager::instance().loadAndRegister(PALETTE_TEXTURE_PATH, TEXNAME_PALETTE);
    if (err) {
        print(*err);
    }
}

void Posterize::process(rl::RenderTexture src, rl::RenderTexture dst) {
#ifndef NDEBUG
    if (!S_ENABLED) {
        Graphics.blit(src, dst);
        return;
    }
#endif
    Shader& mPosterize = ShaderMgr::get("Quantize");
    auto paletteTex = TextureManager::getTexture(TEXNAME_PALETTE);
    mPosterize.setTexture("_Palette", paletteTex);
    mPosterize.setVector2("_PaletteTexSize", rl::Vector2(paletteTex.width, paletteTex.height));
    Graphics.blit(src, dst, mPosterize.get());
}

#ifndef NDEBUG
void Posterize::drawDebug() {
    ImGui::Begin("Posterization");
    ImGui::Checkbox("Active", &S_ENABLED);
    ImGui::End();
}
#endif

}  // namespace whal

#include "Posterize.h"

#include "Gfx/RaylibUtil.h"
#include "Gfx/ShaderManager.h"
#include "Gfx/Texture.h"
#include "Settings.h"
#include "Sys/System.h"
#include "Util/Print.h"
#include "raylib.h"

namespace whal {

Posterize::Posterize() {
    auto err = TextureManager::instance().loadAndRegister(PALETTE_TEXTURE_PATH, TEXNAME_PALETTE);
    if (err) {
        print(*err);
    }
}

void Posterize::process(rl::RenderTexture src, rl::RenderTexture dst) {
    Shader& mPosterize = ShaderMgr::get("Quantize");
    mPosterize.setTexture("_Palette", TextureManager::getTexture(TEXNAME_PALETTE));
    Graphics.blit(src, dst, mPosterize.get());
}

}  // namespace whal

#include "Posterize.h"

#include "Gfx/RaylibUtil.h"
#include "Util/Print.h"
#include "raylib.h"

namespace whal {

Posterize::Posterize() : mPosterize("whalengine/src/Shader/Quantize.glsl") {
    auto err = TextureManager::instance().loadAndRegister(PALETTE_TEXTURE_PATH, TEXNAME_PALETTE);
    if (err) {
        print(*err);
        mPosterize.invalidate();
    }
}

void Posterize::process(rl::RenderTexture src, rl::RenderTexture dst) {
    assert(mPosterize.isValid());

    mPosterize.setTexture("_Palette", TextureManager::getTexture(TEXNAME_PALETTE));
    Graphics.blit(src, dst, mPosterize.get());
}

}  // namespace whal

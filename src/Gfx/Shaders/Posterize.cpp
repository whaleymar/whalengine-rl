#include "Posterize.h"

#include "Gfx/RaylibUtil.h"
#include "Util/Print.h"
#include "raylib.h"

namespace whal {

Posterize::Posterize() : mPosterize("", "whalengine/src/Shader/quantize.glsl") {
    auto err = TextureManager::instance().loadAndRegister(PALETTE_TEXTURE_PATH, TEXNAME_PALETTE);
    if (err) {
        print(*err);
        mPosterize.invalidate();
    }
}

void Posterize::process(rl::RenderTexture src, rl::RenderTexture dst) {
    assert(mPosterize.isValid());

    // can't use Blit because Texture uniforms MUST be set after BeginTextureMode
    // TODO Shader.set<UniformType> should QUEUE operations in the renderer, and the queue can be processed with a custom `BeginTextureMode` variant
    // (that also activates the shader)
    rl::BeginTextureMode(dst);
    rl::BeginShaderMode(mPosterize.get());
    mPosterize.setTexture("_Palette", TextureManager::getTexture(TEXNAME_PALETTE));
    gfx::DrawRenderTexture(src);
    rl::EndShaderMode();
    rl::EndTextureMode();
}

}  // namespace whal

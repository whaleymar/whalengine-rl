#include "Tonemap.h"

#include "Gfx/RaylibUtil.h"
#include "raylib.h"

namespace whal {

Tonemap::Tonemap() : mToneMap("", "whalengine/src/Shader/toneMapping.glsl") {}

void Tonemap::process(rl::RenderTexture src, rl::RenderTexture dst) {
    assert(mToneMap.isValid());
    Graphics.blit(src, dst, mToneMap.get());
}

}  // namespace whal

#include "Tonemap.h"

#include "Gfx/RaylibUtil.h"
#include "Gfx/ShaderManager.h"
#include "Sys/System.h"
#include "raylib.h"

namespace whal {

Tonemap::Tonemap() {}

void Tonemap::process(rl::RenderTexture src, rl::RenderTexture dst) {
    Graphics.blit(src, dst, ShaderMgr::get("ToneMap").get());
}

}  // namespace whal

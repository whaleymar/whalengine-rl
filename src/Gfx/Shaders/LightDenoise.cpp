#include "LightDenoise.h"

#include "Gfx/RaylibUtil.h"
#include "Sys/System.h"
#include "raylib.h"

namespace whal {

static bool isPowerOfTwo(s32 n) {
    return (n & (n - 1)) == 0;
}

LightDenoise::LightDenoise() : mDenoise("", "whalengine/src/Shader/blur.glsl") {}

// If I don't want blur, this just sets alpha to 1 for all values, otherwise multiplication gets weird
// LightDenoise::LightDenoise() : mDenoise("", "whalengine/src/Shader/lightpassthrough.glsl") {}

void LightDenoise::process(rl::RenderTexture src, rl::RenderTexture dst) {
    assert(mDenoise.isValid());
    assert(isPowerOfTwo(dst.texture.width / src.texture.width) && isPowerOfTwo(dst.texture.height / src.texture.height) &&
           "dst must be bigger than src by a power of 2");
    assert((dst.texture.width / src.texture.width) == (dst.texture.height / src.texture.height) && "src and dst must have same width:height ratios");

    mDenoise.setVector2("iResolution", rl::Vector2(src.texture.width, src.texture.height));

    rl::RenderTexture tmpSrc = Graphics.getTemporaryRT(src.texture, rl::TEXTURE_FILTER_BILINEAR);

    // blur the src with the shader
    // this is only performant when src is around quarter resolution
    Graphics.blit(src, tmpSrc, mDenoise.get());

    // incrementally upscale by powers of 2 until we reach the dst resolution
    s32 srcWidth = src.texture.width;
    s32 srcHeight = src.texture.height;
    while (srcWidth != dst.texture.width) {
        // scale to a new texture twice as big
        rl::RenderTexture swap =
            Graphics.getTemporaryRT(srcWidth * 2, srcHeight * 2, static_cast<rl::PixelFormat>(src.texture.format), rl::TEXTURE_FILTER_BILINEAR);
        Graphics.blit(tmpSrc, swap);

        // multiply width
        srcWidth *= 2;
        srcHeight *= 2;

        // Swap & Release
        Graphics.releaseTemporaryRT(tmpSrc);
        tmpSrc = swap;
    }

    // Copy to dst
    Graphics.blit(tmpSrc, dst);
    Graphics.releaseTemporaryRT(tmpSrc);
}

}  // namespace whal

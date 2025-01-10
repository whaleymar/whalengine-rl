#include "LightDenoise.h"

#include "Gfx/RaylibUtil.h"
#include "Sys/System.h"
#include "raylib.h"

namespace whal {

// #ifndef NDEBUG
// static bool isPowerOfTwo(s32 n) {
//     return (n & (n - 1)) == 0;
// }
// static bool isMultipleOf(s32 bigger, s32 smaller) {
//     return bigger % smaller == 0;
// }
// #endif

LightDenoise::LightDenoise() : mDenoise("", "whalengine/src/Shader/blur.glsl") {}

// If I don't want blur, this just sets alpha to 1 for all values, otherwise multiplication gets weird
// LightDenoise::LightDenoise() : mDenoise("", "whalengine/src/Shader/lightpassthrough.glsl") {}

void LightDenoise::process(rl::RenderTexture src, rl::RenderTexture dst) {
    assert(mDenoise.isValid());
    // assert(isPowerOfTwo(dst.texture.width / src.texture.width) && isPowerOfTwo(dst.texture.height / src.texture.height) &&
    //        "dst must be bigger than src by a power of 2");
    // assert(isMultipleOf(dst.texture.width, src.texture.width) && isMultipleOf(dst.texture.height, src.texture.height) &&
    //        "dst's dimensions must be an integer multiple of src's");
    // assert((dst.texture.width / src.texture.width) == (dst.texture.height / src.texture.height) && "src and dst must have same width:height
    // ratios");

    mDenoise.setVector2("iResolution", rl::Vector2(src.texture.width, src.texture.height));

    // rl::RenderTexture tmpSrc = Graphics.getTemporaryRT(src.texture, rl::TEXTURE_FILTER_BILINEAR);
    rl::RenderTexture tmpSrc = Graphics.getTemporaryRT(src.texture, rl::TEXTURE_FILTER_POINT);

    // blur the src with the shader
    // this is only performant when src is around quarter resolution
    Graphics.blit(src, tmpSrc, mDenoise.get());

    // incrementally upscale by powers of 2 until we reach the dst resolution
    s32 srcWidth = src.texture.width;
    s32 srcHeight = src.texture.height;
    while (srcWidth != dst.texture.width) {
        // scale to a new texture twice as big
        s32 dstWidth = srcWidth * 2;
        s32 dstHeight = srcHeight * 2;

        if (dstWidth > dst.texture.width || dstHeight > dst.texture.height) {
            // We overshot the destination dimensions, because the dst:source size ratio was not a power of two.
            // This can happen if the window is resized by the user.
            // I'll just stretch what we have to match the requested dst size.
            // It will look weird if the destination doesn't have the same aspect ratio as the source, but
            // this shader shouldn't be responsible for enforcing that. I can do it at the game or engine level.
            dstWidth = dst.texture.width;
            dstHeight = dst.texture.height;
        }

        rl::RenderTexture swap =
            // Graphics.getTemporaryRT(dstWidth, dstHeight, static_cast<rl::PixelFormat>(src.texture.format), rl::TEXTURE_FILTER_BILINEAR);
            Graphics.getTemporaryRT(dstWidth, dstHeight, static_cast<rl::PixelFormat>(src.texture.format), rl::TEXTURE_FILTER_POINT);
        Graphics.blit(tmpSrc, swap);

        // multiply width
        srcWidth = dstWidth;
        srcHeight = dstHeight;

        // Swap & Release
        Graphics.releaseTemporaryRT(tmpSrc);
        tmpSrc = swap;
    }

    // Copy to dst
    Graphics.blit(tmpSrc, dst);
    Graphics.releaseTemporaryRT(tmpSrc);
}

}  // namespace whal

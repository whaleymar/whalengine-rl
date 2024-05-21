#include "Transform.h"

#include "Settings.h"

namespace whal {

Transform2D Transform2D::texels(s32 x, s32 y) {
    return Transform2D({x * PIXELS_PER_TEXEL, y * PIXELS_PER_TEXEL});
}

Transform2D Transform2D::tiles(s32 x, s32 y) {
    return Transform2D({x * PIXELS_PER_TILE, y * PIXELS_PER_TILE});
}

}  // namespace whal

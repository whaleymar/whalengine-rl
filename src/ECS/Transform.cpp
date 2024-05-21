#include "Transform.h"

#include "Settings.h"

namespace whal {

Transform Transform::texels(s32 x, s32 y) {
    return Transform({x * PIXELS_PER_TEXEL, y * PIXELS_PER_TEXEL});
}

Transform Transform::tiles(s32 x, s32 y) {
    return Transform({x * PIXELS_PER_TILE, y * PIXELS_PER_TILE});
}

}  // namespace whal

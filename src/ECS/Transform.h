#pragma once

#include "Util/Vector.h"

namespace whal {

enum Facing : u8 {
    Left = 0,
    Right = 1,
};

// entity position in pixels
struct Transform2D {
    Vector2i position;
    f32 rotationDegrees = 0.0;     // counterclockwise
    Facing facing = Facing::Left;  // draw calls flipped if facing right
    bool isManuallyMoved = true;   // tells physics system to sync collider position

    static Transform2D texels(s32 x, s32 y);
    static Transform2D tiles(s32 x, s32 y);
};

}  // namespace whal

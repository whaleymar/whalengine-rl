#pragma once

namespace rl {
typedef struct Rectangle Rectangle;
}

#include "Util/Vector.h"

namespace whal {

struct Frame {
    Frame() = default;
    Frame(rl::Rectangle rect);
    Frame(Vector2i, Vector2i);
    Vector2i atlasPosition;
    Vector2i size;
};

}  // namespace whal

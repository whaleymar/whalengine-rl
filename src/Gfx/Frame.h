#pragma once

typedef struct Rectangle Rectangle;

#include "Util/Vector.h"

namespace whal {

struct Frame {
    Frame() = default;
    Frame(Rectangle rect);
    Frame(Vector2i, Vector2i);
    Vector2i atlasPosition;
    Vector2i size;
};

}  // namespace whal

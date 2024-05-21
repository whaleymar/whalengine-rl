#pragma once

#include "Util/Vector.h"
namespace whal {

struct Segment {
    Vector2i origin;
    Vector2i delta;

    Segment(Vector2i delta_);
    Segment(Vector2i origin_, Vector2i delta_);
};

}  // namespace whal

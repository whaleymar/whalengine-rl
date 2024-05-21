#include "Physics/Collision/Segment.h"

namespace whal {

Segment::Segment(Vector2i delta_) : delta(delta_){};
Segment::Segment(Vector2i origin_, Vector2i delta_) : origin(origin_), delta(delta_){};

}  // namespace whal

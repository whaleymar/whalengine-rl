#include "Frame.h"

#include <raylib.h>

namespace whal {

Frame::Frame(rl::Rectangle rect) : atlasPosition(rect.x, rect.y), size(rect.width, rect.height) {}

Frame::Frame(Vector2i atlasPosition, Vector2i dimensions) : atlasPosition(atlasPosition), size(dimensions) {}

}  // namespace whal

#pragma once

#include <vector>
#include "Util/Vector.h"

namespace whal {

struct TileMap;

// A path is a sequential list of movement steps.
// Each step is a tile movement in an ordinal direction
struct Path {
    std::vector<Vector2i> tiles;
    Vector2i start;
    Vector2i target;
};

Path findPath(u32 entityID, const Vector2i startWorldPosition, const Vector2i targetWorldPosition, const TileMap& level, s32 height);

}  // namespace whal

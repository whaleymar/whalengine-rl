#pragma once

#include <memory>
#include <vector>
#include "Util/Vector.h"

namespace whal {

struct TileMap;

struct TileMapLayer {
    Vector2i sizeTiles;
    std::vector<s32> ids;
    std::shared_ptr<TileMap> tilemap;
    std::vector<bool> collisionMask;
    s32 chunkSize = 16;
    bool isYSorted = false;
};

}  // namespace whal

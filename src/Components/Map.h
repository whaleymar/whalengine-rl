#pragma once

#include <memory>
#include <optional>
#include <vector>
#include "Util/Vector.h"

namespace whal {

struct TileMap;

struct TileMapLayer {
    Vector2i sizeTiles;
    std::vector<s32> ids;
    std::shared_ptr<TileMap> tilemap;
    std::vector<bool> collisionMask;
    std::optional<rl::Texture> overlay = std::nullopt;
    s32 chunkSize = 16;
    bool isYSorted = false;
};

// Corresponds to a Tiled level
struct TileMapLevel {
    std::vector<std::vector<bool>> navGrid;  // true == no obstacle at tile
};

}  // namespace whal

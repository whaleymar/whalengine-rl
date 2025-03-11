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
    std::vector<bool> occlusionMask;
    std::string overlayTex = "";
    s32 chunkSize = 16;
    s32 zOffset = 0;
    bool isYSorted = false;
};

// Corresponds to a Tiled level
struct TileMapLevel {
    std::vector<std::vector<bool>> navGrid;  // true == no obstacle at tile
};

// a "pointer" to an entity in the map
struct TileMapEntity {
    std::string mapFile;
    std::string entityName;
};

}  // namespace whal

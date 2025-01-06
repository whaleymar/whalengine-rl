#pragma once

#include "Map/ComponentFactory.h"
#include "Util/Vector.h"

namespace whal {

struct TileMap;

struct TileMapLayer {
    Vector2i sizeTiles;
    std::vector<s32> ids;
    std::shared_ptr<TileMap> tilemap;
    std::vector<bool> collisionMask;
    std::string overlayTex = "";
    s32 chunkSize = 16;
    bool isYSorted = false;
};

// Corresponds to a Tiled level
struct TileMapLevel {
    std::vector<std::vector<bool>> navGrid;  // true == no obstacle at tile
};

// a "pointer" to an entity in the map
struct TileMapEntity : ISerialize<TileMapEntity, ComponentFactory> {
    std::string mapFile;
    std::string entityName;
};
REGISTER_SERIALIZE(TileMapEntity);

}  // namespace whal

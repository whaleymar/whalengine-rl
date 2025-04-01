#pragma once

#include <memory>
#include <vector>
#include "Components/Transform.h"
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

// a "pointer" to an entity in the map
struct TileMapEntityDescriptor {
    std::string mapFile;
    std::string entityName;
};

struct TileMapObject {
    Transform initialTransform;
    std::string mapFile;
};

}  // namespace whal

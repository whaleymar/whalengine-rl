#pragma once

#include <memory>
#include "Systems/Graphics/Common.h"

namespace whal {

struct TileMap;

// TODO delete
// Tiles can cache their render info since they don't move
struct Tile {
    gfx::EntityPreRenderInfo renderInfo;
    bool wasDrawnLastFrame = false;
};

// TODO rename file to TileMapLayer
struct TileMapLayer {
    Vector2i sizeTiles;
    std::vector<s32> ids;
    std::shared_ptr<TileMap> tilemap;
    std::vector<bool> collisionMask;
    s32 chunkSize = 16;
    bool isYSorted = false;
};

}  // namespace whal

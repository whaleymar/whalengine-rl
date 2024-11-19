#pragma once

#include "Systems/Graphics/Common.h"

namespace whal {

// Tiles can cache their render info since they don't move
struct Tile {
    gfx::EntityRenderInfo renderInfo;
};

}  // namespace whal

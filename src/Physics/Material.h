#pragma once

#include "Util/Types.h"

namespace whal {

enum class WorldMaterial : u8 { None, Dirt, Rock, Soft, Wood, Grass, Water, Metal };

const char* toString(WorldMaterial material);

}  // namespace whal

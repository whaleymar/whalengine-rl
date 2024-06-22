#pragma once

#include "Util/Types.h"

namespace whal {

enum class WorldMaterial : u8 { None, Dirt, Rock, Soft, Wood, Grass, Water, Metal, Rubber };

namespace WhalMaterial {

const char* toString(WorldMaterial material);
f32 bounciness(WorldMaterial material);

}  // namespace WhalMaterial

}  // namespace whal

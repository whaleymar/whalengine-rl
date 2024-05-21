#pragma once

#include "Util/Types.h"

namespace whal {

enum class Material : u8 { None, Dirt, Rock, Soft, Wood, Grass, Water, Metal };

const char* toString(Material material);

}  // namespace whal

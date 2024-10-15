#pragma once

#include "Util/Types.h"

namespace whal {

struct GfxFlags {
    enum Flags : u32 {
        Bloom = 1,
        Glow = 1 << 1,
    };

    u32 flags;
};

}  // namespace whal

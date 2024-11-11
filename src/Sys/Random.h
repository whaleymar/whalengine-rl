#pragma once

#include "Util/Types.h"

namespace whal {

struct System;

class RNGManager {
public:
    friend System;

    RNGManager() = default;
    f32 uniform() const;

    f32 range(f32 lower, f32 upper) const;
    s32 range(s32 lower, s32 upperExclusive) const;

private:
    RNGManager(const RNGManager&) = delete;
    void operator=(const RNGManager&) = delete;
};

}  // namespace whal

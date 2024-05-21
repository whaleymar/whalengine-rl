#pragma once

#include <random>

#include "Util/Types.h"

namespace whal {

struct System;

class RNG {
public:
    friend System;

    f32 uniform();

private:
    RNG();

    RNG(const RNG&) = delete;
    void operator=(const RNG&) = delete;

    std::mt19937 mGenerator;
    std::uniform_real_distribution<f32> mDistribution;
};

}  // namespace whal

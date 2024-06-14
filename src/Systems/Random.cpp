#include "Random.h"

#include <random>

namespace whal {

// doing this in cpp file because <random> is HUGE and I don't want it to be included by everything that includes System
f32 RNG::uniform() {
    static std::mt19937 generator(std::random_device{}());
    static std::uniform_real_distribution<f32> distribution(0.0, 1.0);
    return distribution(generator);
}

}  // namespace whal

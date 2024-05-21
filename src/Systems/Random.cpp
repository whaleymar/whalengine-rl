#include "Random.h"

namespace whal {

RNG::RNG() : mGenerator(std::random_device{}()), mDistribution(0.0, 1.0) {}

f32 RNG::uniform() {
    return mDistribution(mGenerator);
}

}  // namespace whal

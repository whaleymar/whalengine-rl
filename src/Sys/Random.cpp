#include "Random.h"

#include <cstring>
#include <ctime>
#include "Util/Types.h"

u32 xorshift32(u32& state) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

float fastRandomFloat(u32& state) {
    u32 randomBits = xorshift32(state) & 0x007FFFFF;  // Use only the lower 23 bits for the mantissa
    u32 floatBits = 0x3F800000 | randomBits;          // Set the exponent to 127 (0x3F800000)
    f32 result;
    std::memcpy(&result, &floatBits, sizeof(result));  // Bitcast to float
    return result - 1.0f;                              // Adjust the range to [0, 1)
}

static u32 RANDOM_STATE = static_cast<u32>(std::time(nullptr));

namespace whal {

f32 RNG::uniform() {
    return fastRandomFloat(RANDOM_STATE);
}

}  // namespace whal

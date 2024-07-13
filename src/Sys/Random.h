#pragma once

typedef float f32;

namespace whal {

struct System;

class RNG {
public:
    friend System;

    f32 uniform();

private:
    RNG() = default;

    RNG(const RNG&) = delete;
    void operator=(const RNG&) = delete;
};

}  // namespace whal

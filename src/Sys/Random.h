#pragma once

typedef float f32;

namespace whal {

struct System;

class RNG {
public:
    friend System;

    f32 uniform() const;

    f32 range(f32 lower, f32 upper) const;
    s32 range(s32 lower, s32 upperExclusive) const;

private:
    RNG() = default;

    RNG(const RNG&) = delete;
    void operator=(const RNG&) = delete;
};

}  // namespace whal

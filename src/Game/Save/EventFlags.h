#pragma once

#include "Util/Types.h"

namespace EventFlags {

enum Flag : u64 {
    HasMagicHat = 1,
};

void set(Flag flag);
void reset(Flag flag);
void resetAll();
bool check(Flag flag);

}  // namespace EventFlags

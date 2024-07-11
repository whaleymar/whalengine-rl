#include "EventFlags.h"

namespace EventFlags {

static u64 S_FLAGS = 0;

void set(Flag flag) {
    S_FLAGS |= flag;
}

void reset(Flag flag) {
    S_FLAGS &= ~flag;
}

bool check(Flag flag) {
    return (S_FLAGS & flag) > 0;
}

void resetAll() {
    S_FLAGS = 0;
}

}  // namespace EventFlags

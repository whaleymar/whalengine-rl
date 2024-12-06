#pragma once

#include "Map/ComponentFactory.h"
#include "Util/ISerialize.h"
#include "Util/Types.h"

namespace whal {

// could be interesting:
// time/function for how fast I arrive at top speed
// jump trajectory

struct BufferedInput {
    s16 framesLeft = 0;
    bool isActive = false;

    void buffer();
    void consume();
    void reset();
    void notUsed();
};

struct PlayerControl : ISerialize<PlayerControl, ComponentFactory> {
    f32 moveSpeed = 80;
};

struct Jumper : ISerialize<Jumper, ComponentFactory> {
    bool isTryingJump() const;
    bool canJump() const;

    // RESEARCH jumpHeight param instead?
    f32 jumpInitialVelocity = 130;
    f32 jumpSecondsMax = 1.25;
    f32 coyoteTimeSecondsMax = 0.1;

    // state:
    f32 jumpSecondsRemaining = 0;
    f32 coyoteSecondsRemaining = 0;
    BufferedInput buffer;
    bool isJumping = false;
};
REGISTER_SERIALIZE(Jumper)

}  // namespace whal

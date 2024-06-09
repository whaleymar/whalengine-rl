#pragma once

#include "Util/Types.h"

namespace whal {

// could be interesting:
// time/function for how fast I arrive at top speed
// jump trajectory
// coyote time window

struct BufferedInput {
    s16 framesLeft = 0;
    bool isActive = false;

    void buffer();
    void consume();
    void reset();
    void notUsed();
};

struct PlayerControl {
    PlayerControl(f32 moveSpeed_ = 80);

    f32 moveSpeed;
};

// tag that says player can freely move in any direction
// an entity shouldn't have this component + rigidbody
struct FreeControl {};

struct Jumper {
    Jumper() = default;
    Jumper(f32 jumpInitialVelocity, f32 jumpSecondsMax, f32 coyoteTimeSecondsMax);

    bool isTryingJump() const;
    bool canJump() const;

    // RESEARCH jumpHeight param instead?
    f32 jumpInitialVelocity = 124;
    f32 jumpSecondsMax = 1.25;
    f32 coyoteTimeSecondsMax = 0.1;

    // state:
    f32 jumpSecondsRemaining = 0;
    f32 coyoteSecondsRemaining = 0;
    BufferedInput buffer;
    bool isJumping = false;
};

}  // namespace whal

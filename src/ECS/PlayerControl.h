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

struct PlayerControlRB {
    PlayerControlRB(f32 moveSpeed_ = 80, f32 jumpHeight_ = 200);
    f32 moveSpeed;   // should be somewhere else... this component should be a tag ?
    f32 jumpHeight;  // not used currently
    BufferedInput jumpBuffer;

    bool isJumping() const;
    bool canJump() const;
};

struct PlayerControlFree {
    PlayerControlFree(f32 moveSpeed_ = 80);
    f32 moveSpeed;
};

}  // namespace whal

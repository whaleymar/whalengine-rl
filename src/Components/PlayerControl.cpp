#include "PlayerControl.h"

namespace whal {

constexpr s16 N_FRAMES_JUMP = 10;

void BufferedInput::buffer() {
    framesLeft = N_FRAMES_JUMP;
    isActive = false;
}

void BufferedInput::consume() {
    framesLeft = 0;
    isActive = true;
}

void BufferedInput::reset() {
    framesLeft = 0;
    isActive = false;
}

void BufferedInput::notUsed() {
    framesLeft--;
}

PlayerControl::PlayerControl(f32 moveSpeed_) : moveSpeed(moveSpeed_) {}

bool Jumper::isTryingJump() const {
    return buffer.isActive;
}

bool Jumper::canJump() const {
    return buffer.framesLeft > 0;
}

Jumper::Jumper(f32 jumpInitialVelocity_, f32 jumpSecondsMax_, f32 coyoteTimeSecondsMax_)
    : jumpInitialVelocity(jumpInitialVelocity_), jumpSecondsMax(jumpSecondsMax_), coyoteTimeSecondsMax(coyoteTimeSecondsMax_) {}

}  // namespace whal

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

PlayerControlRB::PlayerControlRB(f32 moveSpeed_, f32 jumpHeight_) : moveSpeed(moveSpeed_), jumpHeight(jumpHeight_){};

bool PlayerControlRB::isJumping() const {
    return jumpBuffer.isActive;
}

bool PlayerControlRB::canJump() const {
    return jumpBuffer.framesLeft > 0;
}

PlayerControlFree::PlayerControlFree(f32 moveSpeed_) : moveSpeed(moveSpeed_){};

}  // namespace whal

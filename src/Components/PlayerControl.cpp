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

bool Jumper::isTryingJump() const {
    return buffer.isActive;
}

bool Jumper::canJump() const {
    return buffer.framesLeft > 0;
}

}  // namespace whal

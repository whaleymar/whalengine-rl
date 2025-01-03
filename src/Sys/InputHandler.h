#pragma once

#include "Components/Transform.h"
#include "Gfx/Coordinates.h"
#include "Util/Vector.h"

namespace whal {

struct System;

enum class InputType : u64 {
    LEFT = 1,
    RIGHT = 1 << 1,
    UP = 1 << 2,
    DOWN = 1 << 3,
    JUMP = 1 << 4,
    PAUSE = 1 << 5,
    QUIT = 1 << 6,
    M1 = 1 << 7,
    OK = 1 << 8,
    DEBUG = 1 << 9,
    GROWX = 1 << 10,
    GROWY = 1 << 11,
    SHRINKX = 1 << 12,
    SHRINKY = 1 << 13,
    MUSICTEST = 1 << 14,
    RELOADSCENE = 1 << 15,
    TIMETEST = 1 << 16,
    KILLPLAYER = 1 << 17,
    AIM = 1 << 18
};

class InputHandler {
public:
    friend System;

    InputHandler() = default;
    void update();
    void set(InputType input);
    void reset(InputType input);
    void loadMappings() const;
    void useJump();
    bool isOn(InputType input) const;
    Vector2i getMoveNormal() const;
    Direction getDirection() const;

    void disableInputs(u64 mask);
    void enableInputs(u64 mask);
    bool isInputEnabled(InputType input) const;

    void disableMovement() { mIsMovementEnabled = false; }
    void enableMovement() { mIsMovementEnabled = true; }
    bool isMovementEnabled() const { return mIsMovementEnabled; }
    void disableJumping() { mIsJumpingEnabled = false; }
    void enableJumping() { mIsJumpingEnabled = true; }
    bool isJumpingEnabled() const { return mIsJumpingEnabled; }
    bool isJumpAvailable() const { return mIsJumpPressed; }

    Vector2i getMouseScreen() const { return mMouseScreenPosition; }
    Vector2f getMouseWorld() const { return screenToWorldCoords(mMouseScreenPosition); }

private:
    InputHandler(const InputHandler&) = delete;
    void operator=(const InputHandler&) = delete;

    u64 mFlags = 0;
    u64 mDeactivationFlags = 0;  // for inputs which are disabled
    Vector2i mMouseScreenPosition;
    bool mIsJumpPressed = false;
    bool mIsMovementEnabled = true;
    bool mIsJumpingEnabled = true;
};

}  // namespace whal

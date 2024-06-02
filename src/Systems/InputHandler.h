#pragma once

#include <unordered_map>
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
    SHOOT = 1 << 7,
    OK = 1 << 8,
    DEBUG = 1 << 9,
    GROWX = 1 << 10,
    GROWY = 1 << 11,
    SHRINKX = 1 << 12,
    SHRINKY = 1 << 13,
    MUSICTEST = 1 << 14,
    RELOADSCENE = 1 << 15,
    TIMETEST = 1 << 16,
    KILLPLAYER = 1 << 17
};

class InputHandler {
public:
    friend System;

    void update();
    void set(InputType input);
    void reset(InputType input);
    void loadMappings() const;
    void useJump();
    bool isOn(InputType input);

    bool isJumpAvailable() const { return mIsJumpPressed; }

    inline static std::unordered_map<int, InputType> KeyMap;
    inline static std::unordered_map<int, InputType> MouseMap;
    inline static Vector2i MousePosition;

private:
    InputHandler();
    InputHandler(const InputHandler&) = delete;
    void operator=(const InputHandler&) = delete;

    u64 mFlags = 0;
    bool mIsJumpPressed = false;
};

}  // namespace whal

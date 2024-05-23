#include "InputHandler.h"

#include <raylib.h>

#include "Game/Events.h"
#include "Gfx/Coordinates.h"
#include "System.h"
#include "Util/Print.h"

namespace whal {

InputHandler::InputHandler() {
    loadMappings();
}

void InputHandler::update() {
    for (auto [key, inputType] : KeyMap) {
        if (IsKeyPressed(key)) {
            set(inputType);
        } else if (IsKeyReleased(key)) {
            reset(inputType);
        }
    }
    MousePosition = fromRaylibInt(GetMousePosition());
    for (auto [mouseButton, inputType] : MouseMap) {
        if (IsMouseButtonPressed(mouseButton)) {
            set(inputType);
        } else if (IsKeyReleased(mouseButton)) {
            reset(inputType);
        }
    }
}

void InputHandler::set(InputType input) {
    switch (input) {
    case InputType::LEFT:
        mIsLeft = true;
        break;

    case InputType::RIGHT:
        mIsRight = true;
        break;

    case InputType::UP:
        mIsUp = true;
        break;

    case InputType::DOWN:
        mIsDown = true;
        break;

    case InputType::JUMP:
        mIsJump = true;
        mIsJumpPressed = true;
        break;

    case InputType::PAUSE:
        mIsPause = mIsPause != true;
        break;

    case InputType::QUIT:
        mIsQuit = true;
        break;

    case InputType::SHOOT: {
        System::eventMgr.triggerEvent(Event::SHOOT_EVENT, screenToWorldCoords(MousePosition));
        break;
    }

#ifndef NDEBUG
    case InputType::DEBUG:
        mIsDebug = mIsDebug != true;
        break;

    case InputType::SHRINKX:
        mIsShrinkX = true;
        break;

    case InputType::SHRINKY:
        mIsShrinkY = true;
        break;

    case InputType::GROWX:
        mIsGrowX = true;
        break;

    case InputType::GROWY:
        mIsGrowY = true;
        break;

    case InputType::MUSICTEST:
        mIsMusicTest = true;
        break;

    case InputType::RELOADSCENE:
        mIsReloadScene = true;
        break;

    case InputType::TIMETEST:
        mIsTimeTest = true;
        break;
    case InputType::KILLPLAYER:
        mIsKillPlayer = true;
        break;
#endif
    }
}

void InputHandler::reset(InputType input) {
    switch (input) {
    case InputType::LEFT:
        mIsLeft = false;
        break;

    case InputType::RIGHT:
        mIsRight = false;
        break;

    case InputType::UP:
        mIsUp = false;
        break;

    case InputType::DOWN:
        mIsDown = false;
        break;

    case InputType::JUMP:
        mIsJump = false;
        mIsJumpPressed = false;
        break;

#ifndef NDEBUG
    case InputType::SHRINKX:
        mIsShrinkX = false;
        break;

    case InputType::SHRINKY:
        mIsShrinkY = false;
        break;

    case InputType::GROWX:
        mIsGrowX = false;
        break;

    case InputType::GROWY:
        mIsGrowY = false;
        break;

    case InputType::MUSICTEST:
        mIsMusicTest = false;
        break;

    case InputType::RELOADSCENE:
        mIsReloadScene = false;
        break;

    case InputType::TIMETEST:
        mIsTimeTest = false;
        break;
    case InputType::KILLPLAYER:
        mIsKillPlayer = false;
        break;
#endif

    default:
        break;
    }
}

void InputHandler::loadMappings() const {
    // EVENTUALLY load from file once i have, like, menus working

    KeyMap.clear();
    MouseMap.clear();

    KeyMap.insert({KEY_A, InputType::LEFT});
    KeyMap.insert({KEY_D, InputType::RIGHT});
    KeyMap.insert({KEY_W, InputType::UP});
    KeyMap.insert({KEY_S, InputType::DOWN});
    KeyMap.insert({KEY_SPACE, InputType::JUMP});
    KeyMap.insert({KEY_ESCAPE, InputType::QUIT});

    MouseMap.insert({MOUSE_BUTTON_LEFT, InputType::SHOOT});

#ifndef NDEBUG
    KeyMap.insert({KEY_ZERO, InputType::DEBUG});
    KeyMap.insert({KEY_M, InputType::MUSICTEST});
    KeyMap.insert({KEY_R, InputType::RELOADSCENE});
    KeyMap.insert({KEY_T, InputType::TIMETEST});
    KeyMap.insert({KEY_K, InputType::KILLPLAYER});
//     KeyMap.insert({GLFW_KEY_LEFT, InputType::SHRINKX});
//     KeyMap.insert({GLFW_KEY_RIGHT, InputType::GROWX});
//     KeyMap.insert({GLFW_KEY_DOWN, InputType::SHRINKY});
//     KeyMap.insert({GLFW_KEY_UP, InputType::GROWY});
#endif
}

void InputHandler::useJump() {
    mIsJumpPressed = false;
}

}  // namespace whal

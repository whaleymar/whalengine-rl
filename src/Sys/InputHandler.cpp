#include "InputHandler.h"

#include <raylib.h>

#include "Events/Events.h"
// #include "Gfx/Coordinates.h"
#include "System.h"

// #include "Game/Events.h"

namespace whal {

InputHandler::InputHandler() {
    loadMappings();
}

void InputHandler::update() {
    for (auto [key, inputType] : KeyMap) {
        if (!isInputEnabled(inputType)) {
            continue;
        }

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
#ifndef NDEBUG
    case InputType::DEBUG:
        if (isOn(input)) {
            mFlags &= ~static_cast<u64>(input);
        } else {
            mFlags |= static_cast<u64>(input);
        }
        break;
#endif

    case InputType::SHOOT: {
        mFlags |= static_cast<u64>(input);
        break;
    }

    case InputType::JUMP:
        mIsJumpPressed = true;
        mFlags |= static_cast<u64>(input);
        break;
    default: {
        mFlags |= static_cast<u64>(input);
    }
    }

    System::eventMgr.triggerEvent<ButtonPressEvent>(input);
    System::eventMgr.triggerEvent<ButtonPressOrReleaseEvent>(input, true);
}

void InputHandler::reset(InputType input) {
    switch (input) {
    case InputType::JUMP:
        mIsJumpPressed = false;
        mFlags &= ~static_cast<u64>(input);
        break;

    case InputType::DEBUG:
        break;

    default:
        mFlags &= ~static_cast<u64>(input);
    }
    System::eventMgr.triggerEvent<ButtonPressOrReleaseEvent>(input, false);
}

void InputHandler::loadMappings() const {
    // EVENTUALLY load from file once i have, like, menus working

    KeyMap.clear();
    MouseMap.clear();

    // KeyMap.insert({KEY_A, InputType::LEFT});
    // KeyMap.insert({KEY_D, InputType::RIGHT});
    // KeyMap.insert({KEY_W, InputType::UP});
    // KeyMap.insert({KEY_S, InputType::DOWN});
    // KeyMap.insert({KEY_SPACE, InputType::JUMP});
    // KeyMap.insert({KEY_ESCAPE, InputType::PAUSE});
    // KeyMap.insert({KEY_ENTER, InputType::OK});
    // KeyMap.insert({KEY_LEFT, InputType::LEFT});
    // KeyMap.insert({KEY_RIGHT, InputType::RIGHT});
    // KeyMap.insert({KEY_UP, InputType::UP});
    // KeyMap.insert({KEY_DOWN, InputType::DOWN});
    //
    // MouseMap.insert({MOUSE_BUTTON_LEFT, InputType::SHOOT});

    KeyMap.insert({KEY_UP, InputType::UP});
    KeyMap.insert({KEY_RIGHT, InputType::RIGHT});
    KeyMap.insert({KEY_DOWN, InputType::DOWN});
    KeyMap.insert({KEY_LEFT, InputType::LEFT});
    KeyMap.insert({KEY_C, InputType::JUMP});
    KeyMap.insert({KEY_X, InputType::AIM});
    KeyMap.insert({KEY_ESCAPE, InputType::PAUSE});
    KeyMap.insert({KEY_ENTER, InputType::OK});

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

bool InputHandler::isOn(InputType input) const {
    auto mask = static_cast<u64>(input);
    // return (mFlags & mask & !mDeactivationFlags) == mask; // BROKEN
    return (mFlags & mask) == mask;
}

Vector2i InputHandler::getMoveNormal() const {
    Vector2i moveNormal{};
    if (isOn(InputType::UP)) {
        moveNormal.e[1] = 1;
    } else if (isOn(InputType::DOWN)) {
        moveNormal.e[1] = -1;
    }

    if (isOn(InputType::RIGHT)) {
        moveNormal.e[0] = 1;
    } else if (isOn(InputType::LEFT)) {
        moveNormal.e[0] = -1;
    }

    return moveNormal;
}

Direction InputHandler::getDirection() const {
    if (isOn(InputType::UP)) {
        if (isOn(InputType::RIGHT)) {
            return Direction::NE;
        } else if (isOn(InputType::LEFT)) {
            return Direction::NW;
        } else {
            return Direction::N;
        }
    } else if (isOn(InputType::DOWN)) {
        if (isOn(InputType::RIGHT)) {
            return Direction::SE;
        } else if (isOn(InputType::LEFT)) {
            return Direction::SW;
        } else {
            return Direction::S;
        }
    } else if (isOn(InputType::RIGHT)) {
        return Direction::E;
    } else if (isOn(InputType::LEFT)) {
        return Direction::W;
    } else {
        return Direction::Neutral;
    }
}

void InputHandler::disableInputs(u64 mask) {
    mDeactivationFlags |= mask;
}

void InputHandler::enableInputs(u64 mask) {
    mDeactivationFlags &= !mask;
}

bool InputHandler::isInputEnabled(InputType input) const {
    auto mask = static_cast<u64>(input);
    return !((mask & mDeactivationFlags) > 0);
}

}  // namespace whal

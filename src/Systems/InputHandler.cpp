#include "InputHandler.h"

#include <raylib.h>

#include "Game/Events.h"
#include "Gfx/Coordinates.h"
#include "System.h"

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
        // RESEARCH whoever listens for this event should listen for MOUSE event
        System::eventMgr.triggerEvent(Event::SHOOT_EVENT, screenToWorldCoords(MousePosition));
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
    System::eventMgr.triggerEvent(Event::BUTTON_EVENT, input);
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
    KeyMap.insert({KEY_P, InputType::PAUSE});  // TODO should be escape
    KeyMap.insert({KEY_ENTER, InputType::OK});
    KeyMap.insert({KEY_LEFT, InputType::LEFT});
    KeyMap.insert({KEY_RIGHT, InputType::RIGHT});
    KeyMap.insert({KEY_UP, InputType::UP});
    KeyMap.insert({KEY_DOWN, InputType::DOWN});

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

bool InputHandler::isOn(InputType input) {
    auto mask = static_cast<u64>(input);
    return (mFlags & mask) == mask;
}

}  // namespace whal

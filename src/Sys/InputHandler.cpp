#include "InputHandler.h"

#include <raylib.h>
#include <unordered_map>

#include "Events/Events.h"
#include "System.h"

namespace whal {

inline static std::unordered_map<int, InputType> S_KEYMAP;
inline static std::unordered_map<int, InputType> S_MOUSEMAP;

void InputHandler::update() {
    for (auto [key, inputType] : S_KEYMAP) {
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
    for (auto [mouseButton, inputType] : S_MOUSEMAP) {
        if (IsMouseButtonPressed(mouseButton)) {
            set(inputType);
        } else if (IsMouseButtonReleased(mouseButton)) {
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

    case InputType::JUMP:
        mIsJumpPressed = true;
        mFlags |= static_cast<u64>(input);
        break;
    default: {
        mFlags |= static_cast<u64>(input);
    }
    }

    System::event.emit<ButtonPressEvent>(input);
    System::event.emit<ButtonPressOrReleaseEvent>(input, true);
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
    System::event.emit<ButtonPressOrReleaseEvent>(input, false);
}

void InputHandler::loadMappings() const {
    // TODO EVENTUALLY load from file once i have, like, menus working
    // TODO should put default mappings in Game/ directory

    S_MOUSEMAP.insert({MOUSE_BUTTON_LEFT, InputType::M1});
    S_KEYMAP.insert({KEY_UP, InputType::UP});
    S_KEYMAP.insert({KEY_W, InputType::UP});
    S_KEYMAP.insert({KEY_RIGHT, InputType::RIGHT});
    S_KEYMAP.insert({KEY_D, InputType::RIGHT});
    S_KEYMAP.insert({KEY_DOWN, InputType::DOWN});
    S_KEYMAP.insert({KEY_S, InputType::DOWN});
    S_KEYMAP.insert({KEY_LEFT, InputType::LEFT});
    S_KEYMAP.insert({KEY_A, InputType::LEFT});
    // S_KEYMAP.insert({KEY_C, InputType::JUMP});
    // S_KEYMAP.insert({KEY_X, InputType::AIM});
    S_KEYMAP.insert({KEY_ESCAPE, InputType::PAUSE});
    S_KEYMAP.insert({KEY_ENTER, InputType::OK});

#ifndef NDEBUG
    S_KEYMAP.insert({KEY_ZERO, InputType::DEBUG});
    S_KEYMAP.insert({KEY_M, InputType::MUSICTEST});
    S_KEYMAP.insert({KEY_R, InputType::RELOADSCENE});
    S_KEYMAP.insert({KEY_T, InputType::TIMETEST});
    S_KEYMAP.insert({KEY_K, InputType::KILLPLAYER});
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
        moveNormal.y = 1;
    } else if (isOn(InputType::DOWN)) {
        moveNormal.y = -1;
    }

    if (isOn(InputType::RIGHT)) {
        moveNormal.x = 1;
    } else if (isOn(InputType::LEFT)) {
        moveNormal.x = -1;
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

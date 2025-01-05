#include "InputHandler.h"

#include <raylib.h>
#include <unordered_map>

#include "Events/Events.h"
#include "System.h"
#include "Util/STL_reduce.h"

namespace whal {

enum class InputState {
    Off,
    Pressed,
    Held,
    Released,
};

static std::unordered_map<std::string, InputState> S_NAME_TO_STATE;
static std::unordered_map<std::string, std::vector<InputCode>> S_NAME_TO_INPUTS;
static std::unordered_set<InputCode> S_DISABLED_INPUTS;
static constexpr u32 KEYBOARD_ENUM_OFFSET = 512;                     // the max val is in the 300s
static constexpr u32 MOUSE_ENUM_OFFSET = KEYBOARD_ENUM_OFFSET + 64;  // like 6 values

InputCode GetInputCode(rl::KeyboardKey key) {
    return static_cast<InputCode>(key);
}

InputCode GetInputCode(rl::MouseButton button) {
    return KEYBOARD_ENUM_OFFSET + static_cast<InputCode>(button);
}

InputCode GetInputCode(rl::GamepadButton button) {
    return MOUSE_ENUM_OFFSET + static_cast<InputCode>(button);
}

void InputHandler::update() {
    // emits input events
    for (const auto& [name, inputCodes] : S_NAME_TO_INPUTS) {
        InputState state = InputState::Off;
        for (auto inputCode : inputCodes) {
            if (S_DISABLED_INPUTS.contains(inputCode)) {
                continue;
            }

            if (isPressed(inputCode) && state == InputState::Off) {
                // Pressed can override Off
                state = InputState::Pressed;
                S_NAME_TO_STATE[name] = InputState::Pressed;
            } else if (isHeld(inputCode) && (state == InputState::Off || state == InputState::Pressed || state == InputState::Released)) {
                // Held can override Off and Pressed (i.e. if 2 buttons are registered to "jump", then if button 1 is held, pressing button 2 will not
                // caused a "pressed" event) and releasing button 2 won't cause a "released" event
                state = InputState::Held;
                S_NAME_TO_STATE[name] = InputState::Held;
            } else if (isReleased(inputCode) && state == InputState::Off) {
                state = InputState::Released;
                S_NAME_TO_STATE[name] = InputState::Released;
            } else if ((isPressed(inputCode) && state == InputState::Released) || (isReleased(inputCode) && state == InputState::Pressed)) {
                // press + release on same frame => held
                state = InputState::Held;
                S_NAME_TO_STATE[name] = InputState::Held;
            }
        }

        if (state == InputState::Pressed) {
            Event.emit<evt::Input, InputEvent>(InputEvent{
                .name = name,
                .isPressed = true,
                .isHeld = false,
                .isReleased = false,
            });
        } else if (state == InputState::Held) {
            Event.emit<evt::Input, InputEvent>(InputEvent{
                .name = name,
                .isPressed = false,
                .isHeld = true,
                .isReleased = false,
            });

        } else if (state == InputState::Released) {
            Event.emit<evt::Input, InputEvent>(InputEvent{
                .name = name,
                .isPressed = false,
                .isHeld = false,
                .isReleased = true,
            });
        }
    }
    mMouseScreenPosition = Vector2i(rl::GetMousePosition());
}

void InputHandler::loadMappings(const InputPair mappings[], s32 count) const {
    for (s32 i = 0; i < count; i++) {
        add(mappings[i].name, mappings[i].code);
    }
}

bool InputHandler::isPressed(const std::string& name) const {
    auto it = S_NAME_TO_STATE.find(name);
    if (it == S_NAME_TO_STATE.end()) {
        return false;
    }
    return it->second == InputState::Pressed;
}

bool InputHandler::isPressed(InputCode code) const {
    if (code < KEYBOARD_ENUM_OFFSET) {
        // keyboard key
        return rl::IsKeyPressed(static_cast<rl::KeyboardKey>(code));
    } else if (code < MOUSE_ENUM_OFFSET) {
        // mouse button
        return rl::IsMouseButtonPressed(static_cast<rl::MouseButton>(code - KEYBOARD_ENUM_OFFSET));
    } else {
        // gamepad button
        // return rl::IsGamepadButtonPressed(?, static_cast<rl::GamePadButton>(code - MOUSE_ENUM_OFFSET));
    }
    return false;
}

bool InputHandler::isPressed(rl::KeyboardKey key) const {
    return rl::IsKeyPressed(key);
}

bool InputHandler::isPressed(rl::MouseButton button) const {
    return rl::IsMouseButtonPressed(button);
}

// bool InputHandler::isPressed(rl::GamepadButton button) const {}

bool InputHandler::isReleased(const std::string& name) const {
    auto it = S_NAME_TO_STATE.find(name);
    if (it == S_NAME_TO_STATE.end()) {
        return false;
    }
    return it->second == InputState::Released;
}

bool InputHandler::isReleased(InputCode code) const {
    if (code < KEYBOARD_ENUM_OFFSET) {
        // keyboard key
        return rl::IsKeyReleased(static_cast<rl::KeyboardKey>(code));
    } else if (code < MOUSE_ENUM_OFFSET) {
        // mouse button
        return rl::IsMouseButtonReleased(static_cast<rl::MouseButton>(code - KEYBOARD_ENUM_OFFSET));
    } else {
        // gamepad button
        // return rl::IsGamepadButtonReleased(?, static_cast<rl::GamePadButton>(code - MOUSE_ENUM_OFFSET));
    }
    return false;
}

bool InputHandler::isReleased(rl::KeyboardKey key) const {
    return rl::IsKeyReleased(key);
}

bool InputHandler::isReleased(rl::MouseButton button) const {
    return rl::IsMouseButtonReleased(button);
}

// bool InputHandler::isReleased(rl::GamepadButton button) const;

bool InputHandler::isHeld(const std::string& name) const {
    auto it = S_NAME_TO_STATE.find(name);
    if (it == S_NAME_TO_STATE.end()) {
        return false;
    }
    return it->second == InputState::Held;
}

bool InputHandler::isHeld(InputCode code) const {
    if (code < KEYBOARD_ENUM_OFFSET) {
        // keyboard key
        // return rl::IsKeyPressedRepeat(static_cast<rl::KeyboardKey>(code));
        return rl::IsKeyDown(static_cast<rl::KeyboardKey>(code)) && !rl::IsKeyPressed(static_cast<rl::KeyboardKey>(code));
    } else if (code < MOUSE_ENUM_OFFSET) {
        // mouse button
        const rl::MouseButton button = static_cast<rl::MouseButton>(code - KEYBOARD_ENUM_OFFSET);
        return isHeld(button);
    } else {
        // gamepad button
        // return rl::IsGamepadButtonPressed(?, static_cast<rl::GamePadButton>(code - MOUSE_ENUM_OFFSET));
    }
    return false;
}

bool InputHandler::isHeld(rl::KeyboardKey key) const {
    // return rl::IsKeyPressedRepeat(key);
    return rl::IsKeyDown(key) && !rl::IsKeyPressed(key);
}

bool InputHandler::isHeld(rl::MouseButton button) const {
    return rl::IsMouseButtonDown(button) && !rl::IsMouseButtonPressed(button);
}

// bool isHeld(rl::GamepadButton button) const;

bool InputHandler::isOn(const std::string& name) const {
    return isPressed(name) || isHeld(name);
}

bool InputHandler::isOn(InputCode code) const {
    return isPressed(code) || isHeld(code);
}

bool InputHandler::isOn(rl::KeyboardKey key) const {
    return isPressed(key) || isHeld(key);
}

bool InputHandler::isOn(rl::MouseButton button) const {
    return isPressed(button) || isHeld(button);
}

// bool InputHandler::isOn(rl::GamepadButton button) const;

void InputHandler::add(const std::string& name, InputCode code) const {
    if (S_NAME_TO_INPUTS.contains(name)) {
        // check to make sure it's not already added
        auto it = stl::find(S_NAME_TO_INPUTS[name].begin(), S_NAME_TO_INPUTS[name].end(), code);
        if (it == S_NAME_TO_INPUTS[name].end()) {
            S_NAME_TO_INPUTS[name].push_back(code);
        }
    } else {
        S_NAME_TO_INPUTS[name] = {code};
    }
}

void InputHandler::add(const std::string& name, rl::KeyboardKey key) const {
    add(name, GetInputCode(key));
}

void InputHandler::add(const std::string& name, rl::MouseButton button) const {
    add(name, GetInputCode(button));
}

// void InputHandler::add(const std::string& name, rl::GamepadButton button);

void InputHandler::remove(const std::string& name) const {
    S_NAME_TO_INPUTS.erase(name);
}

void InputHandler::disable(InputCode code) const {
    S_DISABLED_INPUTS.insert(code);
}

void InputHandler::disable(rl::KeyboardKey key) const {
    S_DISABLED_INPUTS.insert(GetInputCode(key));
}

void InputHandler::disable(rl::MouseButton button) const {
    S_DISABLED_INPUTS.insert(GetInputCode(button));
}

void InputHandler::enable(InputCode code) const {
    S_DISABLED_INPUTS.erase(code);
}

void InputHandler::enable(rl::KeyboardKey key) const {
    S_DISABLED_INPUTS.erase(GetInputCode(key));
}

void InputHandler::enable(rl::MouseButton button) const {
    S_DISABLED_INPUTS.erase(GetInputCode(button));
}

}  // namespace whal

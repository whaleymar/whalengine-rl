#include "InputHandler.h"

#include <raylib.h>
#include <rfl/json.hpp>
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

static const char* rlKeyToString(rl::KeyboardKey key);
static const char* rlMouseToString(rl::MouseButton button);

InputCode GetInputCode(rl::KeyboardKey key) {
    return static_cast<InputCode>(key);
}

InputCode GetInputCode(rl::MouseButton button) {
    return KEYBOARD_ENUM_OFFSET + static_cast<InputCode>(button);
}

InputCode GetInputCode(rl::GamepadButton button) {
    return MOUSE_ENUM_OFFSET + static_cast<InputCode>(button);
}

std::string InputCodeToString(InputCode code) {
    if (code < KEYBOARD_ENUM_OFFSET) {
        // keyboard key
        rl::KeyboardKey key = static_cast<rl::KeyboardKey>(code);
        return rlKeyToString(key);
    } else if (code < MOUSE_ENUM_OFFSET) {
        // mouse button
        rl::MouseButton button = static_cast<rl::MouseButton>(code - KEYBOARD_ENUM_OFFSET);
        return rlMouseToString(button);
    } else {
        // gamepad button
        // return rl::IsGamepadButtonPressed(?, static_cast<rl::GamePadButton>(code - MOUSE_ENUM_OFFSET));
    }
    return "N/A";
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

std::string InputHandler::toString() const {
    return rfl::json::write(S_NAME_TO_INPUTS);
}

bool InputHandler::fromString(const std::string& data) {
    auto inputMap = rfl::json::read<std::unordered_map<std::string, std::vector<InputCode>>>(data);
    if (inputMap) {
        S_NAME_TO_INPUTS = inputMap.value();
        return false;
    }
    return true;
}

const char* rlKeyToString(rl::KeyboardKey key) {
    switch (key) {
    case rl::KEY_NULL:
        return "Null";
    case rl::KEY_APOSTROPHE:
        return "Apostrophe";
    case rl::KEY_COMMA:
        return "Comma";
    case rl::KEY_MINUS:
        return "Minus";
    case rl::KEY_PERIOD:
        return "Period";
    case rl::KEY_SLASH:
        return "Slash";
    case rl::KEY_ZERO:
        return "Zero";
    case rl::KEY_ONE:
        return "One";
    case rl::KEY_TWO:
        return "Two";
    case rl::KEY_THREE:
        return "Three";
    case rl::KEY_FOUR:
        return "Four";
    case rl::KEY_FIVE:
        return "Five";
    case rl::KEY_SIX:
        return "Six";
    case rl::KEY_SEVEN:
        return "Seven";
    case rl::KEY_EIGHT:
        return "Eight";
    case rl::KEY_NINE:
        return "Nine";
    case rl::KEY_SEMICOLON:
        return "Semicolon";
    case rl::KEY_EQUAL:
        return "Equal";
    case rl::KEY_A:
        return "A";
    case rl::KEY_B:
        return "B";
    case rl::KEY_C:
        return "C";
    case rl::KEY_D:
        return "D";
    case rl::KEY_E:
        return "E";
    case rl::KEY_F:
        return "F";
    case rl::KEY_G:
        return "G";
    case rl::KEY_H:
        return "H";
    case rl::KEY_I:
        return "I";
    case rl::KEY_J:
        return "J";
    case rl::KEY_K:
        return "K";
    case rl::KEY_L:
        return "L";
    case rl::KEY_M:
        return "M";
    case rl::KEY_N:
        return "N";
    case rl::KEY_O:
        return "O";
    case rl::KEY_P:
        return "P";
    case rl::KEY_Q:
        return "Q";
    case rl::KEY_R:
        return "R";
    case rl::KEY_S:
        return "S";
    case rl::KEY_T:
        return "T";
    case rl::KEY_U:
        return "U";
    case rl::KEY_V:
        return "V";
    case rl::KEY_W:
        return "W";
    case rl::KEY_X:
        return "X";
    case rl::KEY_Y:
        return "Y";
    case rl::KEY_Z:
        return "Z";
    case rl::KEY_LEFT_BRACKET:
        return "Left Bracket";
    case rl::KEY_BACKSLASH:
        return "Backslash";
    case rl::KEY_RIGHT_BRACKET:
        return "Right Bracket";
    case rl::KEY_GRAVE:
        return "Grave";
    case rl::KEY_SPACE:
        return "Space";
    case rl::KEY_ESCAPE:
        return "Escape";
    case rl::KEY_ENTER:
        return "Enter";
    case rl::KEY_TAB:
        return "Tab";
    case rl::KEY_BACKSPACE:
        return "Backspace";
    case rl::KEY_INSERT:
        return "Insert";
    case rl::KEY_DELETE:
        return "Delete";
    case rl::KEY_RIGHT:
        return "Right";
    case rl::KEY_LEFT:
        return "Left";
    case rl::KEY_DOWN:
        return "Down";
    case rl::KEY_UP:
        return "Up";
    case rl::KEY_PAGE_UP:
        return "Page Up";
    case rl::KEY_PAGE_DOWN:
        return "Page Down";
    case rl::KEY_HOME:
        return "Home";
    case rl::KEY_END:
        return "End";
    case rl::KEY_CAPS_LOCK:
        return "Caps Lock";
    case rl::KEY_SCROLL_LOCK:
        return "Scroll Lock";
    case rl::KEY_NUM_LOCK:
        return "Num Lock";
    case rl::KEY_PRINT_SCREEN:
        return "Print Screen";
    case rl::KEY_PAUSE:
        return "Pause";
    case rl::KEY_F1:
        return "F1";
    case rl::KEY_F2:
        return "F2";
    case rl::KEY_F3:
        return "F3";
    case rl::KEY_F4:
        return "F4";
    case rl::KEY_F5:
        return "F5";
    case rl::KEY_F6:
        return "F6";
    case rl::KEY_F7:
        return "F7";
    case rl::KEY_F8:
        return "F8";
    case rl::KEY_F9:
        return "F9";
    case rl::KEY_F10:
        return "F10";
    case rl::KEY_F11:
        return "F11";
    case rl::KEY_F12:
        return "F12";
    case rl::KEY_LEFT_SHIFT:
        return "Left Shift";
    case rl::KEY_LEFT_CONTROL:
        return "Left Control";
    case rl::KEY_LEFT_ALT:
        return "Left Alt";
    case rl::KEY_LEFT_SUPER:
        return "Left Super";
    case rl::KEY_RIGHT_SHIFT:
        return "Right Shift";
    case rl::KEY_RIGHT_CONTROL:
        return "Right Control";
    case rl::KEY_RIGHT_ALT:
        return "Right Alt";
    case rl::KEY_RIGHT_SUPER:
        return "Right Super";
    case rl::KEY_KB_MENU:
        return "Kb Menu";
    case rl::KEY_KP_0:
        return "Kp 0";
    case rl::KEY_KP_1:
        return "Kp 1";
    case rl::KEY_KP_2:
        return "Kp 2";
    case rl::KEY_KP_3:
        return "Kp 3";
    case rl::KEY_KP_4:
        return "Kp 4";
    case rl::KEY_KP_5:
        return "Kp 5";
    case rl::KEY_KP_6:
        return "Kp 6";
    case rl::KEY_KP_7:
        return "Kp 7";
    case rl::KEY_KP_8:
        return "Kp 8";
    case rl::KEY_KP_9:
        return "Kp 9";
    case rl::KEY_KP_DECIMAL:
        return "Kp Decimal";
    case rl::KEY_KP_DIVIDE:
        return "Kp Divide";
    case rl::KEY_KP_MULTIPLY:
        return "Kp Multiply";
    case rl::KEY_KP_SUBTRACT:
        return "Kp Subtract";
    case rl::KEY_KP_ADD:
        return "Kp Add";
    case rl::KEY_KP_ENTER:
        return "Kp Enter";
    case rl::KEY_KP_EQUAL:
        return "Kp Equal";
    case rl::KEY_BACK:
        return "Back";
    case rl::KEY_MENU:
        return "Menu";
    case rl::KEY_VOLUME_UP:
        return "Volume Up";
    case rl::KEY_VOLUME_DOWN:
        return "Volume Down";
    }
}

const char* rlMouseToString(rl::MouseButton button) {
    switch (button) {
    case rl::MOUSE_BUTTON_LEFT:
        return "Mouse Left";
    case rl::MOUSE_BUTTON_RIGHT:
        return "Mouse Right";
    case rl::MOUSE_BUTTON_MIDDLE:
        return "Mouse Middle";
    case rl::MOUSE_BUTTON_SIDE:
        return "Mouse Side";
    case rl::MOUSE_BUTTON_EXTRA:
        return "Mouse Extra";
    case rl::MOUSE_BUTTON_FORWARD:
        return "Mouse Forward";
    case rl::MOUSE_BUTTON_BACK:
        return "Mouse Back";
    }
}

}  // namespace whal

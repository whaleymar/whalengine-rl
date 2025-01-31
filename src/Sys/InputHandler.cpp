#include "InputHandler.h"

#include <raylib.h>
#include <rfl/json.hpp>
#include <unordered_map>

#include "Events/Events.h"
#include "Settings.h"
#include "System.h"
#include "Util/Print.h"
#include "Util/STL_reduce.h"
#include "Util/Saveutil.h"

namespace whal {

enum class InputState {
    Off,
    Pressed,
    Held,
    Released,
};

struct InputAction {
    InputState state;
    f32 strength;
};

// TODO enable unique deadzone values for each input name
// mimic godot API for circle/square deadzone shape

static std::unordered_map<std::string, InputAction> S_NAME_TO_STATE;
static std::unordered_map<std::string, std::vector<InputCode>> S_NAME_TO_INPUTS;
static std::unordered_set<InputCode> S_DISABLED_INPUTS;
static bool S_IS_STOPPED = false;  // flag for if all inputs are disabled

static constexpr u32 KEYBOARD_ENUM_OFFSET = 512;                     // the max val is in the 300s
static constexpr u32 MOUSE_ENUM_OFFSET = KEYBOARD_ENUM_OFFSET + 32;  // like 6 values
static constexpr u32 GAMEPAD_ENUM_OFFSET = MOUSE_ENUM_OFFSET + 32;   // like 18 values

static const char* rlKeyToString(rl::KeyboardKey key);
static const char* rlMouseToString(rl::MouseButton button);
static const char* rlGamepadButtonToString(rl::GamepadButton button, GamepadType controller);
static const char* GamepadAxisToString(whal::GamepadAxis axis);

static constexpr s32 AXIS_ENUM_COUNT = 10;
struct GamepadAxisState {
    enum class Joystick {
        Left,
        Right,
    };

    GamepadAxisState() {
        mState.fill(InputState::Off);
        mStrength.fill(0);
        mStrengthRaw.fill(0);
    }

    // this should only be called once per frame for each axis!!!
    void setStrength(f32 stren, whal::GamepadAxis axis) {
        const s32 ix = getIndex(axis);
        mStrength[ix] = math::abs(stren);
        bool wasOn = mState[ix] == InputState::Pressed || mState[ix] == InputState::Held;
        if (mStrength[ix] > 0) {
            mState[ix] = wasOn ? InputState::Held : InputState::Pressed;
        } else {
            mState[ix] = wasOn ? InputState::Released : InputState::Off;
        }
    }

    void syncStrength(whal::GamepadAxis axis) { setStrength(mStrengthRaw[getIndex(axis)], axis); }
    void setStrengthRaw(f32 stren, whal::GamepadAxis axis) { mStrengthRaw[getIndex(axis)] = math::abs(stren); }
    void setState(InputState s, whal::GamepadAxis axis) { mState[getIndex(axis)] = s; }
    s32 getIndex(whal::GamepadAxis axis) { return static_cast<s32>(axis); }
    f32 getStrength(whal::GamepadAxis axis) { return mStrength[getIndex(axis)]; }
    f32 getStrengthRaw(whal::GamepadAxis axis) { return mStrengthRaw[getIndex(axis)]; }
    InputState getState(whal::GamepadAxis axis) { return mState[getIndex(axis)]; }

    bool isJoystickInDeadzoneCircle(Joystick joystick, f32 deadzone) {
        Vector2f vec = joystick == Joystick::Left ? mLeftJoystickVecRaw : mRightJoystickVecRaw;
        return vec.isZero() || vec.len() <= deadzone;
    }

    bool isJoystickInDeadzoneSquare(Joystick joystick, f32 deadzone, bool isX) {
        Vector2f vec = joystick == Joystick::Left ? mLeftJoystickVecRaw : mRightJoystickVecRaw;
        if (isX) {
            return math::abs(vec.x) <= deadzone;
        } else {
            return math::abs(vec.y) <= deadzone;
        }
    }

    void setRawVector(f32 stren, Joystick joystick, bool isX) {
        if (joystick == Joystick::Left) {
            if (isX) {
                mLeftJoystickVecRaw.x = stren;
            } else {
                mLeftJoystickVecRaw.y = stren;
            }
        } else {
            if (isX) {
                mRightJoystickVecRaw.x = stren;
            } else {
                mRightJoystickVecRaw.y = stren;
            }
        }
    }

    Vector2f getVector(Joystick joystick) {
        if (joystick == Joystick::Left) {
            return mLeftJoystickVecRaw;
        } else {
            return mRightJoystickVecRaw;
        }
    }

private:
    std::array<InputState, AXIS_ENUM_COUNT> mState;
    std::array<f32, AXIS_ENUM_COUNT> mStrength;  // accounts for deadzone (circular deadzone for joysticks)
    std::array<f32, AXIS_ENUM_COUNT> mStrengthRaw;
    Vector2f mLeftJoystickVecRaw;
    Vector2f mRightJoystickVecRaw;
};
static GamepadAxisState S_GAMEPAD_AXIS_STATE;

// static rl::GamepadAxis axisToRL(whal::GamepadAxis axis) {
//     switch (axis) {
//     case whal::GamepadAxis::AXIS_0_MINUS:
//         return rl::GamepadAxis::GAMEPAD_AXIS_LEFT_X;
//     case whal::GamepadAxis::AXIS_0_PLUS:
//         return rl::GamepadAxis::GAMEPAD_AXIS_LEFT_X;
//     case whal::GamepadAxis::AXIS_1_MINUS:
//         return rl::GamepadAxis::GAMEPAD_AXIS_LEFT_Y;
//     case whal::GamepadAxis::AXIS_1_PLUS:
//         return rl::GamepadAxis::GAMEPAD_AXIS_LEFT_Y;
//     case whal::GamepadAxis::AXIS_2_MINUS:
//         return rl::GamepadAxis::GAMEPAD_AXIS_RIGHT_X;
//     case whal::GamepadAxis::AXIS_2_PLUS:
//         return rl::GamepadAxis::GAMEPAD_AXIS_RIGHT_X;
//     case whal::GamepadAxis::AXIS_3_MINUS:
//         return rl::GamepadAxis::GAMEPAD_AXIS_RIGHT_Y;
//     case whal::GamepadAxis::AXIS_3_PLUS:
//         return rl::GamepadAxis::GAMEPAD_AXIS_RIGHT_Y;
//     case whal::GamepadAxis::AXIS_LEFT_TRIGGER:
//         return rl::GamepadAxis::GAMEPAD_AXIS_LEFT_TRIGGER;
//     case whal::GamepadAxis::AXIS_RIGHT_TRIGGER:
//         return rl::GamepadAxis::GAMEPAD_AXIS_RIGHT_TRIGGER;
//     }
// }

InputCode GetInputCode(rl::KeyboardKey key) {
    return static_cast<InputCode>(key);
}

InputCode GetInputCode(rl::MouseButton button) {
    return KEYBOARD_ENUM_OFFSET + static_cast<InputCode>(button);
}

InputCode GetInputCode(rl::GamepadButton button) {
    return MOUSE_ENUM_OFFSET + static_cast<InputCode>(button);
}

InputCode GetInputCode(whal::GamepadAxis axis) {
    return GAMEPAD_ENUM_OFFSET + static_cast<InputCode>(axis);
}

const char* InputCodeToString(InputCode code) {
    if (code < KEYBOARD_ENUM_OFFSET) {
        // keyboard key
        rl::KeyboardKey key = static_cast<rl::KeyboardKey>(code);
        return rlKeyToString(key);
    } else if (code < MOUSE_ENUM_OFFSET) {
        // mouse button
        rl::MouseButton button = static_cast<rl::MouseButton>(code - KEYBOARD_ENUM_OFFSET);
        return rlMouseToString(button);
    } else if (code < GAMEPAD_ENUM_OFFSET) {
        // gamepad button
        rl::GamepadButton button = static_cast<rl::GamepadButton>(code - MOUSE_ENUM_OFFSET);
        return rlGamepadButtonToString(button, Input.getGamepadType());
    } else {
        // gamepad axis
        whal::GamepadAxis axis = static_cast<whal::GamepadAxis>(code - GAMEPAD_ENUM_OFFSET);
        return GamepadAxisToString(axis);
    }
    return "N/A";
}

GamepadType GamepadFromString(const std::string& name) {
    if (name == "Pro Controller") {
        return GamepadType::ProController;
        // } else if (name == "xbox" || name == "x-box") {
        // return GamepadType::XBox;
        // } else if (name == "playstation") {
        // return GamepadType::Playstation;
    } else {
        return GamepadType::Unknown;
    }
}

void InputHandler::update() {
    mMouseScreenPosition = Vector2i(rl::GetMousePosition()) - Vector2i(WINDOW_POS_OS_X, WINDOW_POS_OS_Y);
    mMouseWindowPosition = Vector2i(rl::GetMousePosition());

    if (S_IS_STOPPED) {
        return;
    }

    updateGamepadState();

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
                S_NAME_TO_STATE[name].state = InputState::Pressed;
                S_NAME_TO_STATE[name].strength = getStrength(inputCode);
            } else if (isHeld(inputCode) && (state == InputState::Off || state == InputState::Pressed || state == InputState::Released)) {
                // Held can override Off and Pressed (i.e. if 2 buttons are registered to "jump", then if button 1 is held, pressing button 2 will not
                // caused a "pressed" event) and releasing button 2 won't cause a "released" event
                state = InputState::Held;
                S_NAME_TO_STATE[name].state = InputState::Held;
                S_NAME_TO_STATE[name].strength = getStrength(inputCode);
            } else if (isReleased(inputCode) && state == InputState::Off) {
                state = InputState::Released;
                S_NAME_TO_STATE[name].state = InputState::Released;
                S_NAME_TO_STATE[name].strength = getStrength(inputCode);
            } else if ((isPressed(inputCode) && state == InputState::Released) || (isReleased(inputCode) && state == InputState::Pressed)) {
                // press + release on same frame => held
                state = InputState::Held;
                S_NAME_TO_STATE[name].state = InputState::Held;
                S_NAME_TO_STATE[name].strength = getStrength(inputCode);
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
}

void InputHandler::updateGamepadState() {
    // check gamepads? idk what im doing
    for (s32 i = 0; i < 4; i++) {
        if (rl::IsGamepadAvailable(i)) {
            if (mActiveGamepad != i) {
                print("Setting gamepad to", rl::GetGamepadName(i));
                Input.setActiveGamepad(i);
            }
            break;
        }
    }

    if (!isUsingGamepad()) {
        return;
    }

    // THIS
    // IS
    // A
    // MESS

    // RAW INPUT STRENGTH
    f32 strength = rl::GetGamepadAxisMovement(mActiveGamepad, rl::GamepadAxis::GAMEPAD_AXIS_LEFT_X);
    if (strength > 0) {
        S_GAMEPAD_AXIS_STATE.setStrengthRaw(strength, GamepadAxis::AXIS_0_PLUS);
        S_GAMEPAD_AXIS_STATE.setStrengthRaw(0, GamepadAxis::AXIS_0_MINUS);
    } else {
        S_GAMEPAD_AXIS_STATE.setStrengthRaw(0, GamepadAxis::AXIS_0_PLUS);
        S_GAMEPAD_AXIS_STATE.setStrengthRaw(strength, GamepadAxis::AXIS_0_MINUS);
    }
    S_GAMEPAD_AXIS_STATE.setRawVector(strength, GamepadAxisState::Joystick::Left, true);

    strength = rl::GetGamepadAxisMovement(mActiveGamepad, rl::GamepadAxis::GAMEPAD_AXIS_LEFT_Y);
    if (strength > 0) {
        S_GAMEPAD_AXIS_STATE.setStrengthRaw(strength, GamepadAxis::AXIS_1_PLUS);
        S_GAMEPAD_AXIS_STATE.setStrengthRaw(0, GamepadAxis::AXIS_1_MINUS);
    } else {
        S_GAMEPAD_AXIS_STATE.setStrengthRaw(0, GamepadAxis::AXIS_1_PLUS);
        S_GAMEPAD_AXIS_STATE.setStrengthRaw(strength, GamepadAxis::AXIS_1_MINUS);
    }
    S_GAMEPAD_AXIS_STATE.setRawVector(strength, GamepadAxisState::Joystick::Left, false);

    strength = rl::GetGamepadAxisMovement(mActiveGamepad, rl::GamepadAxis::GAMEPAD_AXIS_RIGHT_X);
    if (strength > 0) {
        S_GAMEPAD_AXIS_STATE.setStrengthRaw(strength, GamepadAxis::AXIS_2_PLUS);
        S_GAMEPAD_AXIS_STATE.setStrengthRaw(0, GamepadAxis::AXIS_2_MINUS);
    } else {
        S_GAMEPAD_AXIS_STATE.setStrengthRaw(0, GamepadAxis::AXIS_2_PLUS);
        S_GAMEPAD_AXIS_STATE.setStrengthRaw(strength, GamepadAxis::AXIS_2_MINUS);
    }
    S_GAMEPAD_AXIS_STATE.setRawVector(strength, GamepadAxisState::Joystick::Right, true);

    strength = rl::GetGamepadAxisMovement(mActiveGamepad, rl::GamepadAxis::GAMEPAD_AXIS_RIGHT_Y);
    if (strength > 0) {
        S_GAMEPAD_AXIS_STATE.setStrengthRaw(strength, GamepadAxis::AXIS_3_PLUS);
        S_GAMEPAD_AXIS_STATE.setStrengthRaw(0, GamepadAxis::AXIS_3_MINUS);
    } else {
        S_GAMEPAD_AXIS_STATE.setStrengthRaw(0, GamepadAxis::AXIS_3_PLUS);
        S_GAMEPAD_AXIS_STATE.setStrengthRaw(strength, GamepadAxis::AXIS_3_MINUS);
    }
    S_GAMEPAD_AXIS_STATE.setRawVector(strength, GamepadAxisState::Joystick::Right, false);

    strength = rl::GetGamepadAxisMovement(mActiveGamepad, rl::GamepadAxis::GAMEPAD_AXIS_LEFT_TRIGGER);
    S_GAMEPAD_AXIS_STATE.setStrengthRaw(strength, GamepadAxis::AXIS_LEFT_TRIGGER);

    strength = rl::GetGamepadAxisMovement(mActiveGamepad, rl::GamepadAxis::GAMEPAD_AXIS_RIGHT_TRIGGER);
    S_GAMEPAD_AXIS_STATE.setStrengthRaw(strength, GamepadAxis::AXIS_RIGHT_TRIGGER);

    // ADJUSTED STRENGTH (accounts for deadzone)
    // for axes 0-3, use a circular deadzone (feels more natural)
    if (mUseCircleDeadzone) {
        bool isOutsideDeadzone = !S_GAMEPAD_AXIS_STATE.isJoystickInDeadzoneCircle(GamepadAxisState::Joystick::Left, mJoystickDeadzone);
        if (isOutsideDeadzone) {
            S_GAMEPAD_AXIS_STATE.syncStrength(whal::GamepadAxis::AXIS_0_PLUS);
            S_GAMEPAD_AXIS_STATE.syncStrength(whal::GamepadAxis::AXIS_1_PLUS);
            S_GAMEPAD_AXIS_STATE.syncStrength(whal::GamepadAxis::AXIS_0_MINUS);
            S_GAMEPAD_AXIS_STATE.syncStrength(whal::GamepadAxis::AXIS_1_MINUS);
        } else {
            S_GAMEPAD_AXIS_STATE.setStrength(0, whal::GamepadAxis::AXIS_0_PLUS);
            S_GAMEPAD_AXIS_STATE.setStrength(0, whal::GamepadAxis::AXIS_1_PLUS);
            S_GAMEPAD_AXIS_STATE.setStrength(0, whal::GamepadAxis::AXIS_0_MINUS);
            S_GAMEPAD_AXIS_STATE.setStrength(0, whal::GamepadAxis::AXIS_1_MINUS);
        }
    } else {
        if (!S_GAMEPAD_AXIS_STATE.isJoystickInDeadzoneSquare(GamepadAxisState::Joystick::Left, mJoystickDeadzone, true)) {
            S_GAMEPAD_AXIS_STATE.syncStrength(whal::GamepadAxis::AXIS_0_MINUS);
            S_GAMEPAD_AXIS_STATE.syncStrength(whal::GamepadAxis::AXIS_0_PLUS);
        } else {
            S_GAMEPAD_AXIS_STATE.setStrength(0, whal::GamepadAxis::AXIS_0_MINUS);
            S_GAMEPAD_AXIS_STATE.setStrength(0, whal::GamepadAxis::AXIS_0_PLUS);
        }

        if (!S_GAMEPAD_AXIS_STATE.isJoystickInDeadzoneSquare(GamepadAxisState::Joystick::Left, mJoystickDeadzone, false)) {
            S_GAMEPAD_AXIS_STATE.syncStrength(whal::GamepadAxis::AXIS_1_MINUS);
            S_GAMEPAD_AXIS_STATE.syncStrength(whal::GamepadAxis::AXIS_1_PLUS);
        } else {
            S_GAMEPAD_AXIS_STATE.setStrength(0, whal::GamepadAxis::AXIS_1_MINUS);
            S_GAMEPAD_AXIS_STATE.setStrength(0, whal::GamepadAxis::AXIS_1_PLUS);
        }
    }

    if (mUseCircleDeadzone) {
        bool isOutsideDeadzone = !S_GAMEPAD_AXIS_STATE.isJoystickInDeadzoneCircle(GamepadAxisState::Joystick::Right, mJoystickDeadzone);
        if (isOutsideDeadzone) {
            S_GAMEPAD_AXIS_STATE.syncStrength(whal::GamepadAxis::AXIS_2_PLUS);
            S_GAMEPAD_AXIS_STATE.syncStrength(whal::GamepadAxis::AXIS_3_PLUS);
            S_GAMEPAD_AXIS_STATE.syncStrength(whal::GamepadAxis::AXIS_2_MINUS);
            S_GAMEPAD_AXIS_STATE.syncStrength(whal::GamepadAxis::AXIS_3_MINUS);
        } else {
            S_GAMEPAD_AXIS_STATE.setStrength(0, whal::GamepadAxis::AXIS_2_PLUS);
            S_GAMEPAD_AXIS_STATE.setStrength(0, whal::GamepadAxis::AXIS_3_PLUS);
            S_GAMEPAD_AXIS_STATE.setStrength(0, whal::GamepadAxis::AXIS_2_MINUS);
            S_GAMEPAD_AXIS_STATE.setStrength(0, whal::GamepadAxis::AXIS_3_MINUS);
        }
    } else {
        if (!S_GAMEPAD_AXIS_STATE.isJoystickInDeadzoneSquare(GamepadAxisState::Joystick::Right, mJoystickDeadzone, true)) {
            S_GAMEPAD_AXIS_STATE.syncStrength(whal::GamepadAxis::AXIS_2_MINUS);
            S_GAMEPAD_AXIS_STATE.syncStrength(whal::GamepadAxis::AXIS_2_PLUS);
        } else {
            S_GAMEPAD_AXIS_STATE.setStrength(0, whal::GamepadAxis::AXIS_2_MINUS);
            S_GAMEPAD_AXIS_STATE.setStrength(0, whal::GamepadAxis::AXIS_2_PLUS);
        }

        if (!S_GAMEPAD_AXIS_STATE.isJoystickInDeadzoneSquare(GamepadAxisState::Joystick::Right, mJoystickDeadzone, false)) {
            S_GAMEPAD_AXIS_STATE.syncStrength(whal::GamepadAxis::AXIS_3_MINUS);
            S_GAMEPAD_AXIS_STATE.syncStrength(whal::GamepadAxis::AXIS_3_PLUS);
        } else {
            S_GAMEPAD_AXIS_STATE.setStrength(0, whal::GamepadAxis::AXIS_3_MINUS);
            S_GAMEPAD_AXIS_STATE.setStrength(0, whal::GamepadAxis::AXIS_3_PLUS);
        }
    }

    if (S_GAMEPAD_AXIS_STATE.getStrengthRaw(whal::GamepadAxis::AXIS_LEFT_TRIGGER) > mJoystickDeadzone) {
        S_GAMEPAD_AXIS_STATE.syncStrength(whal::GamepadAxis::AXIS_LEFT_TRIGGER);
    }

    if (S_GAMEPAD_AXIS_STATE.getStrengthRaw(whal::GamepadAxis::AXIS_RIGHT_TRIGGER) > mJoystickDeadzone) {
        S_GAMEPAD_AXIS_STATE.syncStrength(whal::GamepadAxis::AXIS_RIGHT_TRIGGER);
    }
}

void InputHandler::loadMappings(const InputPair mappings[], s32 count) const {
    for (s32 i = 0; i < count; i++) {
        add(mappings[i].name, mappings[i].code);
    }
}

void InputHandler::resetMappings() const {
    S_NAME_TO_INPUTS.clear();
    S_NAME_TO_STATE.clear();
}

void InputHandler::setActiveGamepad(s32 id) {
    assert(id >= 0 && id < 4 && "Gamepad ID must be 0, 1, 2, or 3");
    mActiveGamepad = id;
    const char* name = rl::GetGamepadName(id);
    mGamepadType = GamepadFromString(name);
}

void InputHandler::setGamepadDeadzone(f32 deadzone) {
    mJoystickDeadzone = deadzone;
}

void InputHandler::setIsGamepadDeadzoneCircle(bool isCircle) {
    mUseCircleDeadzone = isCircle;
}

bool InputHandler::isUsingGamepad() const {
    return rl::IsGamepadAvailable(mActiveGamepad);
}

bool InputHandler::isPressed(const std::string& name) const {
    auto it = S_NAME_TO_STATE.find(name);
    if (it == S_NAME_TO_STATE.end()) {
        return false;
    }
    return it->second.state == InputState::Pressed;
}

bool InputHandler::isPressed(InputCode code) const {
    if (code < KEYBOARD_ENUM_OFFSET) {
        // keyboard key
        return rl::IsKeyPressed(static_cast<rl::KeyboardKey>(code));
    } else if (code < MOUSE_ENUM_OFFSET) {
        // mouse button
        return rl::IsMouseButtonPressed(static_cast<rl::MouseButton>(code - KEYBOARD_ENUM_OFFSET));
    } else if (code < GAMEPAD_ENUM_OFFSET) {
        // gamepad button
        return rl::IsGamepadButtonPressed(mActiveGamepad, static_cast<rl::GamepadButton>(code - MOUSE_ENUM_OFFSET));
    } else {
        // gamepad axis
        whal::GamepadAxis axis = static_cast<whal::GamepadAxis>(code - GAMEPAD_ENUM_OFFSET);
        return S_GAMEPAD_AXIS_STATE.getState(axis) == InputState::Pressed;
    }
    return false;
}

bool InputHandler::isPressed(rl::KeyboardKey key) const {
    return rl::IsKeyPressed(key);
}

bool InputHandler::isPressed(rl::MouseButton button) const {
    return rl::IsMouseButtonPressed(button);
}

bool InputHandler::isPressed(rl::GamepadButton button) const {
    return rl::IsGamepadButtonPressed(mActiveGamepad, button);
}

bool InputHandler::isPressed(whal::GamepadAxis axis) const {
    return S_GAMEPAD_AXIS_STATE.getState(axis) == InputState::Pressed;
}

bool InputHandler::isReleased(const std::string& name) const {
    auto it = S_NAME_TO_STATE.find(name);
    if (it == S_NAME_TO_STATE.end()) {
        return false;
    }
    return it->second.state == InputState::Released;
}

bool InputHandler::isReleased(InputCode code) const {
    if (code < KEYBOARD_ENUM_OFFSET) {
        // keyboard key
        return rl::IsKeyReleased(static_cast<rl::KeyboardKey>(code));
    } else if (code < MOUSE_ENUM_OFFSET) {
        // mouse button
        return rl::IsMouseButtonReleased(static_cast<rl::MouseButton>(code - KEYBOARD_ENUM_OFFSET));
    } else if (code < GAMEPAD_ENUM_OFFSET) {
        // gamepad button
        return rl::IsGamepadButtonReleased(mActiveGamepad, static_cast<rl::GamepadButton>(code - MOUSE_ENUM_OFFSET));
    } else {
        // gamepad axis
        whal::GamepadAxis axis = static_cast<whal::GamepadAxis>(code - GAMEPAD_ENUM_OFFSET);
        return S_GAMEPAD_AXIS_STATE.getState(axis) == InputState::Released;
    }
    return false;
}

bool InputHandler::isReleased(rl::KeyboardKey key) const {
    return rl::IsKeyReleased(key);
}

bool InputHandler::isReleased(rl::MouseButton button) const {
    return rl::IsMouseButtonReleased(button);
}

bool InputHandler::isReleased(rl::GamepadButton button) const {
    return rl::IsGamepadButtonReleased(mActiveGamepad, button);
}

bool InputHandler::isReleased(whal::GamepadAxis axis) const {
    return S_GAMEPAD_AXIS_STATE.getState(axis) == InputState::Released;
}

bool InputHandler::isHeld(const std::string& name) const {
    auto it = S_NAME_TO_STATE.find(name);
    if (it == S_NAME_TO_STATE.end()) {
        return false;
    }
    return it->second.state == InputState::Held;
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
    } else if (code < GAMEPAD_ENUM_OFFSET) {
        // gamepad button
        return rl::IsGamepadButtonPressed(mActiveGamepad, static_cast<rl::GamepadButton>(code - MOUSE_ENUM_OFFSET));
    } else {
        // gamepad axis
        whal::GamepadAxis axis = static_cast<whal::GamepadAxis>(code - GAMEPAD_ENUM_OFFSET);
        return S_GAMEPAD_AXIS_STATE.getState(axis) == InputState::Held;
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

bool InputHandler::isHeld(rl::GamepadButton button) const {
    return rl::IsGamepadButtonDown(mActiveGamepad, button) && !rl::IsGamepadButtonPressed(mActiveGamepad, button);
}

bool InputHandler::isHeld(whal::GamepadAxis axis) const {
    return S_GAMEPAD_AXIS_STATE.getState(axis) == InputState::Held;
}

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

bool InputHandler::isOn(rl::GamepadButton button) const {
    return isPressed(button) || isHeld(button);
}

bool InputHandler::isOn(whal::GamepadAxis axis) const {
    return isPressed(axis) || isHeld(axis);
}

f32 InputHandler::getStrength(const std::string& name) const {
    auto it = S_NAME_TO_STATE.find(name);
    if (it == S_NAME_TO_STATE.end()) {
        return false;
    }
    return it->second.strength;
}

f32 InputHandler::getStrength(InputCode code) const {
    if (code < KEYBOARD_ENUM_OFFSET) {
        // keyboard key
        return rl::IsKeyPressed(static_cast<rl::KeyboardKey>(code)) ? 1 : 0;
    } else if (code < MOUSE_ENUM_OFFSET) {
        // mouse button
        return rl::IsMouseButtonPressed(static_cast<rl::MouseButton>(code - KEYBOARD_ENUM_OFFSET)) ? 1 : 0;
    } else if (code < GAMEPAD_ENUM_OFFSET) {
        // gamepad button
        return rl::IsGamepadButtonPressed(mActiveGamepad, static_cast<rl::GamepadButton>(code - MOUSE_ENUM_OFFSET)) ? 1 : 0;
    } else {
        // gamepad axis
        whal::GamepadAxis axis = static_cast<whal::GamepadAxis>(code - GAMEPAD_ENUM_OFFSET);
        return S_GAMEPAD_AXIS_STATE.getStrength(axis);
    }
    return false;
}

f32 InputHandler::getStrength(rl::KeyboardKey key) const {
    return isOn(key) ? 1.0f : 0.0f;
}

f32 InputHandler::getStrength(rl::MouseButton button) const {
    return isOn(button) ? 1.0f : 0.0f;
}

f32 InputHandler::getStrength(rl::GamepadButton button) const {
    return isOn(button) ? 1.0f : 0.0f;
}

f32 InputHandler::getStrength(whal::GamepadAxis axis) const {
    return S_GAMEPAD_AXIS_STATE.getStrength(axis);
}

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

void InputHandler::add(const std::string& name, rl::GamepadButton button) const {
    add(name, GetInputCode(button));
}

void InputHandler::add(const std::string& name, whal::GamepadAxis axis) const {
    add(name, GetInputCode(axis));
}

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

void InputHandler::disable(rl::GamepadButton button) const {
    S_DISABLED_INPUTS.insert(GetInputCode(button));
}

void InputHandler::disable(whal::GamepadAxis axis) const {
    S_DISABLED_INPUTS.insert(GetInputCode(axis));
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

void InputHandler::enable(rl::GamepadButton button) const {
    S_DISABLED_INPUTS.erase(GetInputCode(button));
}

void InputHandler::enable(whal::GamepadAxis axis) const {
    S_DISABLED_INPUTS.erase(GetInputCode(axis));
}

void InputHandler::stop() const {
    S_IS_STOPPED = true;
}

void InputHandler::resume() const {
    S_IS_STOPPED = false;
}

std::string InputHandler::toString() const {
    return rfl::json::write(sortMap(S_NAME_TO_INPUTS));
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
    return nullptr;
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
    return nullptr;
}

const char* rlGamepadButtonToString(rl::GamepadButton button, GamepadType controller) {
    if (controller == GamepadType::ProController) {
        switch (button) {
        case rl::GAMEPAD_BUTTON_UNKNOWN:
            return "ProCon Unknown";
        case rl::GAMEPAD_BUTTON_LEFT_FACE_UP:
            return "ProCon D-Pad Up";
        case rl::GAMEPAD_BUTTON_LEFT_FACE_RIGHT:
            return "ProCon D-Pad Right";
        case rl::GAMEPAD_BUTTON_LEFT_FACE_DOWN:
            return "ProCon D-Pad Down";
        case rl::GAMEPAD_BUTTON_LEFT_FACE_LEFT:
            return "ProCon D-Pad Left";
        case rl::GAMEPAD_BUTTON_RIGHT_FACE_UP:
            return "ProCon X";
        case rl::GAMEPAD_BUTTON_RIGHT_FACE_RIGHT:
            return "ProCon A";
        case rl::GAMEPAD_BUTTON_RIGHT_FACE_DOWN:
            return "ProCon B";
        case rl::GAMEPAD_BUTTON_RIGHT_FACE_LEFT:
            return "ProCon Y";
        case rl::GAMEPAD_BUTTON_LEFT_TRIGGER_1:
            return "ProCon L";
        case rl::GAMEPAD_BUTTON_LEFT_TRIGGER_2:
            return "ProCon ZL";
        case rl::GAMEPAD_BUTTON_RIGHT_TRIGGER_1:
            return "ProCon R";
        case rl::GAMEPAD_BUTTON_RIGHT_TRIGGER_2:
            return "ProCon ZR";
        case rl::GAMEPAD_BUTTON_MIDDLE_LEFT:
            return "ProCon Minus";
        case rl::GAMEPAD_BUTTON_MIDDLE:
            return "ProCon Home";
        case rl::GAMEPAD_BUTTON_MIDDLE_RIGHT:
            return "ProCon Plus";
        case rl::GAMEPAD_BUTTON_LEFT_THUMB:
            return "ProCon Left Thumb";
        case rl::GAMEPAD_BUTTON_RIGHT_THUMB:
            return "ProCon Right Thumb";
        }

    } else {
        switch (button) {
        case rl::GAMEPAD_BUTTON_UNKNOWN:
            return "Gamepad Unknown";
        case rl::GAMEPAD_BUTTON_LEFT_FACE_UP:
            return "Gamepad Left Face Up";
        case rl::GAMEPAD_BUTTON_LEFT_FACE_RIGHT:
            return "Gamepad Left Face Right";
        case rl::GAMEPAD_BUTTON_LEFT_FACE_DOWN:
            return "Gamepad Left Face Down";
        case rl::GAMEPAD_BUTTON_LEFT_FACE_LEFT:
            return "Gamepad Left Face Left";
        case rl::GAMEPAD_BUTTON_RIGHT_FACE_UP:
            return "Gamepad Right Face Up";
        case rl::GAMEPAD_BUTTON_RIGHT_FACE_RIGHT:
            return "Gamepad Right Face Right";
        case rl::GAMEPAD_BUTTON_RIGHT_FACE_DOWN:
            return "Gamepad Right Face Down";
        case rl::GAMEPAD_BUTTON_RIGHT_FACE_LEFT:
            return "Gamepad Right Face Left";
        case rl::GAMEPAD_BUTTON_LEFT_TRIGGER_1:
            return "Gamepad Left Trigger 1";
        case rl::GAMEPAD_BUTTON_LEFT_TRIGGER_2:
            return "Gamepad Left Trigger 2";
        case rl::GAMEPAD_BUTTON_RIGHT_TRIGGER_1:
            return "Gamepad Right Trigger 1";
        case rl::GAMEPAD_BUTTON_RIGHT_TRIGGER_2:
            return "Gamepad Right Trigger 2";
        case rl::GAMEPAD_BUTTON_MIDDLE_LEFT:
            return "Gamepad Middle Left";
        case rl::GAMEPAD_BUTTON_MIDDLE:
            return "Gamepad Middle";
        case rl::GAMEPAD_BUTTON_MIDDLE_RIGHT:
            return "Gamepad Middle Right";
        case rl::GAMEPAD_BUTTON_LEFT_THUMB:
            return "Gamepad Left Thumb";
        case rl::GAMEPAD_BUTTON_RIGHT_THUMB:
            return "Gamepad Right Thumb";
        }
    }
    return nullptr;
}

const char* GamepadAxisToString(whal::GamepadAxis axis) {
    switch (axis) {
    case whal::GamepadAxis::AXIS_0_MINUS:
        return "Left Stick Left / Joystick 0 Left";
    case whal::GamepadAxis::AXIS_0_PLUS:
        return "Left Stick Right / Joystick 0 Right";
    case whal::GamepadAxis::AXIS_1_MINUS:
        return "Left Stick Up / Joystick 0 Up";
    case whal::GamepadAxis::AXIS_1_PLUS:
        return "Left Stick Down / Joystick 0 Down";
    case whal::GamepadAxis::AXIS_2_MINUS:
        return "Right Stick Left / Joystick 1 Left";
    case whal::GamepadAxis::AXIS_2_PLUS:
        return "Right Stick Right / Joystick 1 Right";
    case whal::GamepadAxis::AXIS_3_MINUS:
        return "Right Stick Up / Joystick 1 Up";
    case whal::GamepadAxis::AXIS_3_PLUS:
        return "Right Stick Down / Joystick 1 Down";
    case whal::GamepadAxis::AXIS_LEFT_TRIGGER:
        return "XBox LT / Playstation L2 / Switch ZL";
    case whal::GamepadAxis::AXIS_RIGHT_TRIGGER:
        return "XBox RT / Playstation R2 / Switch ZR";
    }
}

}  // namespace whal

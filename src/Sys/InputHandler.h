#pragma once

#include <string>
#include "Gfx/Coordinates.h"
#include "Util/Vector.h"
#include "raylib.h"

namespace whal {

// raylib's enum doesn't differentiate between right/left/up/down
// this is based on how Godot does it
enum class GamepadAxis {
    AXIS_0_MINUS,        // Left Stick Left / Joystick 0 Left
    AXIS_0_PLUS,         // Left Stick Right / Joystick 0 Right
    AXIS_1_MINUS,        // Left Stick Up / Joystick 0 Up
    AXIS_1_PLUS,         // Left Stick Down / Joystick 0 Down
    AXIS_2_MINUS,        // Right Stick Left / Joystick 1 Left
    AXIS_2_PLUS,         // Right Stick Right / Joystick 1 Right
    AXIS_3_MINUS,        // Right Stick Up / Joystick 1 Up
    AXIS_3_PLUS,         // Right Stick Down / Joystick 1 Down
    AXIS_LEFT_TRIGGER,   // XBox LT / Playstation L2 / Switch ZL
    AXIS_RIGHT_TRIGGER,  // XBox RT / Playstation R2 / Switch ZR
};

struct System;

using InputCode = u32;

// Get a unique normalized value for any input type
InputCode GetInputCode(rl::KeyboardKey key);
InputCode GetInputCode(rl::MouseButton button);
InputCode GetInputCode(rl::GamepadButton button);
InputCode GetInputCode(whal::GamepadAxis axis);

const char* InputCodeToString(InputCode code);

struct InputEvent {
    std::string_view name;
    bool isPressed;
    bool isHeld;
    bool isReleased;
};

enum class GamepadType {
    Unknown,
    ProController,
    // maybe I'll eventually test these:
    // XBox,
    // Playstation
};

GamepadType GamepadFromString(const std::string& name);

class InputHandler {
public:
    friend System;

    struct InputPair {
        const char* name;
        InputCode code;
    };

    InputHandler() = default;
    void update();
    void loadMappings(const InputPair mappings[], s32 count) const;
    void resetMappings() const;
    void setActiveGamepad(s32 id);
    s32 getActiveGamepad() const { return mActiveGamepad; }
    GamepadType getGamepadType() const { return mGamepadType; }
    bool isUsingGamepad() const;

    Vector2i getMouseScreen() const { return mMouseScreenPosition; }
    Vector2i getMouseWindow() const { return mMouseWindowPosition; }
    Vector2f getMouseWorld() const { return screenToWorldCoords(mMouseScreenPosition); }

    // input pressed this frame
    bool isPressed(const std::string& name) const;
    bool isPressed(InputCode code, f32 deadzone) const;
    bool isPressed(rl::KeyboardKey key) const;
    bool isPressed(rl::MouseButton button) const;
    bool isPressed(rl::GamepadButton button) const;
    bool isPressed(whal::GamepadAxis axis, f32 deadzone = 0.5) const;

    // input released this frame
    bool isReleased(const std::string& name) const;
    bool isReleased(InputCode code, f32 deadzone) const;
    bool isReleased(rl::KeyboardKey key) const;
    bool isReleased(rl::MouseButton button) const;
    bool isReleased(rl::GamepadButton button) const;
    bool isReleased(whal::GamepadAxis axis, f32 deadzone = 0.5) const;

    // Input is active, but not pressed this frame
    bool isHeld(const std::string& name) const;
    bool isHeld(InputCode code, f32 deadzone) const;
    bool isHeld(rl::KeyboardKey key) const;
    bool isHeld(rl::MouseButton button) const;
    bool isHeld(rl::GamepadButton button) const;
    bool isHeld(whal::GamepadAxis axis, f32 deadzone = 0.5) const;

    // Input is pressed or held
    bool isOn(const std::string& name) const;
    bool isOn(InputCode code, f32 deadzone) const;
    bool isOn(rl::KeyboardKey key) const;
    bool isOn(rl::MouseButton button) const;
    bool isOn(rl::GamepadButton button) const;
    bool isOn(whal::GamepadAxis axis, f32 deadzone = 0.5) const;

    // Axis-specific methods
    f32 getStrength(const std::string& name) const;                     // returns value between 0-1 for analog inputs, 0/1 for digital
    f32 getStrength(InputCode code, f32 deadzone) const;                // returns 0/1
    f32 getStrength(rl::KeyboardKey key) const;                         // returns 0/1
    f32 getStrength(rl::MouseButton button) const;                      // returns 0/1
    f32 getStrength(rl::GamepadButton button) const;                    // returns 0/1
    f32 getStrength(whal::GamepadAxis axis, f32 deadzone = 0.5) const;  // returns value between 0-1

    f32 getStrengthRaw(const std::string& name) const;  // returns value between 0-1 for analog inputs, 0/1 for digital
    f32 getStrengthRaw(InputCode code) const;           // returns 0/1

    /*
    Gets an input vector by specifying four actions for the positive and negative X and Y axes.

    This method is useful when getting vector input, such as from a joystick, directional pad, arrows, or WASD. The vector has its length limited to 1
    and has a circular deadzone, which is useful for using vector input as movement.

    By default, the deadzone is automatically calculated from the average of the action deadzones. However, you can override the deadzone to be
    whatever you want (on the range of 0 to 1).
    (description from Godot; this works the same)
    */
    Vector2f getVector(const std::string& negativeX, const std::string& positiveX, const std::string& negativeY, const std::string& positiveY,
                       f32 deadzone = -1.0) const;

    void add(const std::string& name, InputCode code) const;
    void add(const std::string& name, rl::KeyboardKey key) const;
    void add(const std::string& name, rl::MouseButton button) const;
    void add(const std::string& name, rl::GamepadButton button) const;
    void add(const std::string& name, whal::GamepadAxis axis) const;

    void setDeadzone(const std::string& name, f32 deadzone) const;

    void remove(const std::string& name) const;

    void disable(InputCode code) const;
    void disable(rl::KeyboardKey key) const;
    void disable(rl::MouseButton button) const;
    void disable(rl::GamepadButton button) const;
    void disable(whal::GamepadAxis axis) const;

    void enable(InputCode code) const;
    void enable(rl::KeyboardKey key) const;
    void enable(rl::MouseButton button) const;
    void enable(rl::GamepadButton button) const;
    void enable(whal::GamepadAxis axis) const;

    void stop() const;    // disables all inputs. can only be undone by `enable`
    void resume() const;  // if `stop` was called previous, this will resume inputs

    std::string toString() const;
    bool fromString(const std::string& data);  // returns true on error

private:
    InputHandler(const InputHandler&) = delete;
    void operator=(const InputHandler&) = delete;

    void updateGamepadState();

    Vector2i mMouseScreenPosition;
    Vector2i mMouseWindowPosition;  // if there's a mismatch between render and window size, this matches window
    s32 mActiveGamepad = 10;        // raylib supports 4 gamepads (0-3).
    GamepadType mGamepadType = GamepadType::Unknown;
};

}  // namespace whal

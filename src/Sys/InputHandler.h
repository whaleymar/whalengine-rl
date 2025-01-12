#pragma once

#include <string>
#include "Gfx/Coordinates.h"
#include "Util/Vector.h"

namespace whal {

struct System;

using InputCode = u32;

// Get a unique normalized value for any input type
InputCode GetInputCode(rl::KeyboardKey key);
InputCode GetInputCode(rl::MouseButton button);
InputCode GetInputCode(rl::GamepadButton button);

std::string InputCodeToString(InputCode code);

struct InputEvent {
    std::string_view name;
    bool isPressed;
    bool isHeld;
    bool isReleased;
};

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

    Vector2i getMouseScreen() const { return mMouseScreenPosition; }
    Vector2i getMouseWindow() const { return mMouseWindowPosition; }
    Vector2f getMouseWorld() const { return screenToWorldCoords(mMouseScreenPosition); }

    // NEW STUFF
    // input pressed this frame
    bool isPressed(const std::string& name) const;
    bool isPressed(InputCode code) const;
    bool isPressed(rl::KeyboardKey key) const;
    bool isPressed(rl::MouseButton button) const;
    // bool isPressed(rl::GamepadButton button) const;

    // input released this frame
    bool isReleased(const std::string& name) const;
    bool isReleased(InputCode code) const;
    bool isReleased(rl::KeyboardKey key) const;
    bool isReleased(rl::MouseButton button) const;
    // bool isReleased(rl::GamepadButton button) const;

    // Input is active, but not pressed this frame
    bool isHeld(const std::string& name) const;
    bool isHeld(InputCode code) const;
    bool isHeld(rl::KeyboardKey key) const;
    bool isHeld(rl::MouseButton button) const;
    // bool isHeld(rl::GamepadButton button) const;

    // Input is pressed or held
    bool isOn(const std::string& name) const;
    bool isOn(InputCode code) const;
    bool isOn(rl::KeyboardKey key) const;
    bool isOn(rl::MouseButton button) const;
    // bool isOn(rl::GamepadButton button) const;

    void add(const std::string& name, InputCode code) const;
    void add(const std::string& name, rl::KeyboardKey key) const;
    void add(const std::string& name, rl::MouseButton button) const;
    // void add(const std::string& name, rl::GamepadButton button);

    void remove(const std::string& name) const;

    void disable(InputCode code) const;
    void disable(rl::KeyboardKey key) const;
    void disable(rl::MouseButton button) const;

    void enable(InputCode code) const;
    void enable(rl::KeyboardKey key) const;
    void enable(rl::MouseButton button) const;

    std::string toString() const;
    bool fromString(const std::string& data);  // returns true on error

private:
    InputHandler(const InputHandler&) = delete;
    void operator=(const InputHandler&) = delete;

    Vector2i mMouseScreenPosition;
    Vector2i mMouseWindowPosition;  // if there's a mismatch between render and window size, this matches window
};

}  // namespace whal

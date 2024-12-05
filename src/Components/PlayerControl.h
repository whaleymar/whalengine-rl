#pragma once

#include "Map/ComponentFactory.h"
#include "Map/Tiled.h"
#include "Util/Types.h"

namespace whal {

// could be interesting:
// time/function for how fast I arrive at top speed
// jump trajectory

struct BufferedInput {
    s16 framesLeft = 0;
    bool isActive = false;

    void buffer();
    void consume();
    void reset();
    void notUsed();
};

struct PlayerControl : ISerialize<PlayerControl, ComponentFactory> {
    f32 moveSpeed = 80;

    static void loadImpl(ecs::Entity entity, void* data) {
        const LoadContext& ctx = *static_cast<LoadContext*>(data);
        PlayerControl control = entity.has<PlayerControl>() ? entity.get<PlayerControl>() : PlayerControl{};
        tryReadFloat(ctx.values, "speed", &control.moveSpeed);

        entity.add(control);
    }
};

struct Jumper : ISerialize<Jumper, ComponentFactory> {
    Jumper() = default;
    Jumper(f32 jumpInitialVelocity, f32 jumpSecondsMax, f32 coyoteTimeSecondsMax);

    bool isTryingJump() const;
    bool canJump() const;

    // RESEARCH jumpHeight param instead?
    f32 jumpInitialVelocity = 130;
    f32 jumpSecondsMax = 1.25;
    f32 coyoteTimeSecondsMax = 0.1;

    // state:
    f32 jumpSecondsRemaining = 0;
    f32 coyoteSecondsRemaining = 0;
    BufferedInput buffer;
    bool isJumping = false;

    static void loadImpl(ecs::Entity entity, void* data) {
        const LoadContext& ctx = *static_cast<LoadContext*>(data);
        Jumper jumper = entity.has<Jumper>() ? entity.get<Jumper>() : Jumper{};

        tryReadFloat(ctx.values, "jumpInitialVelocity", &jumper.jumpInitialVelocity);
        tryReadFloat(ctx.values, "jumpSecondsMax", &jumper.jumpSecondsMax);
        tryReadFloat(ctx.values, "coyoteTimeSecondsMax", &jumper.coyoteTimeSecondsMax);

        entity.add(jumper);
    }
};

}  // namespace whal

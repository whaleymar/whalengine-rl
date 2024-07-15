#pragma once

#include "Settings.h"
#include "Sys/InputHandler.h"
#include "Sys/System.h"
#include "Systems/Physics.h"

#include "Events/Events.h"
#include "Util/Vector.h"
#include "whalECS/src/ECS.h"

namespace whal {
struct Transform2D;
struct Velocity;
struct RigidBody;
struct PlayerControl;
}  // namespace whal

struct Blaster {
    f32 projectileSpeed = abs(whal::TERMINAL_VELOCITY_Y);  // same as terminal velocity
    f32 shotKnockback = 0;
    f32 projectileLifetimeSeconds = 3.5;
    f32 explosionRadius = FPIXELS_PER_TILE * 2;
    f32 cooldownSeconds = 0.2;
    Vector2i aimDirection = {1, 0};  // TODO enum
    s32 maxShots = 4;
    f32 pushStrength = 150;
    f32 RJAirResistance = 0.25f;
    f32 RJStraightUpAirResistance = 1.0f;
    // Vector2f pushStrengthDefault = {150, 150};
    // Vector2f pushStrengthDownAngle = {150, 150};

    Corrade::Containers::Optional<whal::ecs::Entity> aimReticle = Corrade::Containers::NullOpt;
    f32 cooldownRemaining = 0;
    s32 shotsRemaining = maxShots;
};

struct RocketJumping {
    f32 newAirResistance;

    f32 originalAirResistance;
    u32 silhouetteEventId = 0;
    f32 stateTime = 0.0f;
};

class ProjectileSystem : public whal::ecs::ISystem<whal::PlayerControl, Blaster, whal::Transform2D>,
                         public whal::ecs::IUpdate,
                         public whal::ecs::IMonitorSystem,
                         public whal::ecs::IReactToPause,
                         public whal::IListen<whal::ButtonPressOrReleaseEvent, false, whal::InputType, bool> {
public:
    void update() override;
    void onAdd(const whal::ecs::Entity entity) override;
    void onRemove(const whal::ecs::Entity entity) override;
    void onPause() override {}
    void onUnpause() override;
    void onEvent(whal::ButtonPressOrReleaseEvent, whal::InputType, bool) override;

    static void addAimReticles();
    static void updateFacingDirections(bool isFacingRight);

private:
    inline static bool mIsAimUpdateNeeded = false;
};

class RocketJumpingSystem : public whal::ecs::ISystem<RocketJumping, whal::RigidBody, whal::Transform2D, whal::Velocity>,
                            public whal::ecs::IMonitorSystem,
                            public whal::ecs::IUpdate {
public:
    void onAdd(const whal::ecs::Entity) override;
    void onRemove(const whal::ecs::Entity) override;
    void update() override;
};

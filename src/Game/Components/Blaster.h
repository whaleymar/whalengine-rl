#pragma once

#include "Events/Events.h"
#include "Settings.h"
#include "Systems/InputHandler.h"
#include "Systems/System.h"
#include "Util/Vector.h"
#include "whalECS/src/ECS.h"

namespace whal {
struct Transform2D;
struct Velocity;
struct RigidBody;
struct PlayerControl;
}  // namespace whal

struct Blaster {
    f32 projectileSpeed = 160;  // same as terminal velocity
    f32 shotKnockback = 52;
    f32 projectileLifetimeSeconds = 3.5;
    f32 explosionRadius = FPIXELS_PER_TILE * 2.5;
    f32 cooldownSeconds = 0.25;
    Vector2i aimDirection;  // TODO enum
    s32 maxShots = 4;
    Vector2f pushStrengthDefault = {100, 105};
    Vector2f pushStrengthDownAngle = {100, 150};

    Corrade::Containers::Optional<whal::ecs::Entity> aimReticle = Corrade::Containers::NullOpt;
    f32 cooldownRemaining = 0;
    s32 shotsRemaining = maxShots;
};

struct RocketJumping {
    Vector2f prevFrictionMultiplier;
    u32 silhouetteEventId = 0;
};

class ProjectileSystem : public whal::ecs::ISystem<whal::PlayerControl, Blaster, whal::Transform2D>,
                         public whal::ecs::IUpdate,
                         public whal::ecs::IMonitorSystem,
                         public whal::ecs::IReactToPause,
                         public whal::IListen<whal::ButtonPressOrReleaseEvent, false, whal::InputType, bool> {
public:
    void update() override;
    void onAdd(whal::ecs::Entity entity) override;
    void onRemove(whal::ecs::Entity entity) override;
    void onPause() override {}
    void onUnpause() override;
    void onEvent(whal::ButtonPressOrReleaseEvent, whal::InputType, bool) override;

    static void addAimReticles();
    static void updateFacingDirections(bool isFacingRight);
    static void setIsAiming(bool isAiming) { mIsAiming = isAiming; }
    static bool getIsAiming() { return mIsAiming; }

private:
    inline static bool mIsAiming = false;
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

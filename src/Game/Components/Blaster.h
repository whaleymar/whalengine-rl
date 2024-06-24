#pragma once

#include "Events/Events.h"
#include "Settings.h"
#include "Systems/InputHandler.h"
#include "Systems/System.h"
#include "Util/Vector.h"
#include "whalECS/src/ECS.h"

namespace whal {
struct Transform2D;
struct RigidBody;
struct PlayerControl;
}  // namespace whal

struct Blaster {
    f32 projectileSpeed = 160;  // same as terminal velocity
    f32 shotKnockback = 50;
    f32 projectileLifetimeSeconds = 3.5;
    f32 explosionRadius = FPIXELS_PER_TILE * 2.5;
    Vector2i aimDirection;

    Corrade::Containers::Optional<whal::ecs::Entity> aimReticle = Corrade::Containers::NullOpt;
};

struct RocketJumping {
    Vector2f prevFrictionMultiplier;
};

class ProjectileSystem : public whal::ecs::ISystem<whal::PlayerControl, Blaster, whal::Transform2D>,
                         public whal::ecs::IFixedUpdate,
                         public whal::ecs::IMonitorSystem,
                         public whal::ecs::IReactToPause,
                         public whal::IListen<whal::ButtonPressOrReleaseEvent, false, whal::InputType, bool> {
public:
    void fixedUpdate() override;
    void onAdd(whal::ecs::Entity entity) override {}
    void onRemove(whal::ecs::Entity entity) override;
    void onPause() override {}
    void onUnpause() override;
    void onEvent(whal::InputType, bool) override;

    static void addAimReticles();
    static void updateFacingDirections(bool isFacingRight);
    static void setIsAiming(bool isAiming) { mIsAiming = isAiming; }
    static bool getIsAiming() { return mIsAiming; }

private:
    inline static bool mIsAiming = false;
    inline static bool mIsAimUpdateNeeded = false;
};

class RocketJumpingSystem : public whal::ecs::ISystem<RocketJumping, whal::RigidBody>,
                            public whal::ecs::IMonitorSystem,
                            public whal::IListen<whal::LandingEvent, false, whal::ecs::Entity> {
public:
    void onAdd(const whal::ecs::Entity) override;
    void onRemove(const whal::ecs::Entity) override;
    void onEvent(whal::ecs::Entity) override;
};

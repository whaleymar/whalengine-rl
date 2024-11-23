#pragma once

#include "Events/Events.h"
#include "Physics/HitInfo.h"
#include "Sys/IListen.h"
#include "whalECS/src/ECS.h"

#include "Util/Types.h"

namespace whal {

struct Transform;
struct Velocity;
struct AngularVelocity;
struct HitInfo;

// TODO parameter file
inline constexpr f32 TERMINAL_VELOCITY_Y = -160;

class PhysicsSystem : public ecs::ISystem<Transform, Velocity>, public ecs::IUpdate, public IListen<evt::Collision, true, ecs::Entity, HitInfo> {
public:
    void update() override;
    void onEvent(evt::Collision, ecs::Entity, HitInfo) override;
};

class RotationPhysicsSystem : public ecs::ISystem<Transform, AngularVelocity>, public ecs::IUpdate {
public:
    void update() override;
};

}  // namespace whal

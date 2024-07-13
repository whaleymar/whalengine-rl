#pragma once

#include "Events/Events.h"
#include "Physics/HitInfo.h"
#include "Sys/System.h"
#include "whalECS/src/ECS.h"

#include "Util/Types.h"

namespace whal {

struct Transform2D;
struct Velocity;
struct HitInfo;

inline constexpr f32 TERMINAL_VELOCITY_Y = -160;

class PhysicsSystem : public ecs::ISystem<Transform2D, Velocity>, public ecs::IUpdate, public IListen<CollisionEvent, true, ecs::Entity, HitInfo> {
public:
    void update() override;
    void onEvent(CollisionEvent, ecs::Entity, HitInfo) override;
};

}  // namespace whal

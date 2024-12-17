#pragma once

#include "Events/Events.h"
#include "Sys/IListen.h"
#include "whalECS/src/ECS.h"

namespace whal {

struct Attach;
struct Orbit;
struct Follow;
struct Velocity;
struct Transform;

class AttachSystem : public ecs::ISystem<Attach, Transform>, public ecs::IUpdate {
public:
    void update() override;
};

class OrbitSystem : public ecs::ISystem<Orbit, Transform>, public ecs::IUpdate {
public:
    void update() override;
};

class FollowSystem : public ecs::ISystem<Follow, Velocity, Transform>,
                     public ecs::IUpdate,
                     public ecs::IMonitorSystem,
                     public IListen<evt::Death, true, ecs::Entity> {
public:
    void update() override;
    void onAdd(ecs::Entity entity) override {}
    void onRemove(ecs::Entity entity) override;
    void onEvent(evt::Death, ecs::Entity entity) override;
};

}  // namespace whal

#pragma once

#include "Events/Events.h"
#include "Sys/System.h"
#include "whalECS/src/ECS.h"

namespace whal {

struct Children;
struct Attach;
struct Orbit;
struct Follow;
struct Velocity;
struct Transform2D;

class EntityChildSystem : public ecs::ISystem<Children>, public ecs::IMonitorSystem, public IListen<DeathEvent, true, ecs::Entity> {
public:
    void onAdd(ecs::Entity entity) override {}
    void onRemove(ecs::Entity entity) override;
    void onEvent(DeathEvent, ecs::Entity entity) override;
};

class AttachSystem : public ecs::ISystem<Attach, Transform2D>, public ecs::IUpdate {
public:
    void update() override;
};

class OrbitSystem : public ecs::ISystem<Orbit, Transform2D>, public ecs::IUpdate {
public:
    void update() override;
};

class FollowSystem : public ecs::ISystem<Follow, Velocity, Transform2D>,
                     public ecs::IUpdate,
                     public ecs::IMonitorSystem,
                     public IListen<DeathEvent, true, ecs::Entity> {
public:
    void update() override;
    void onAdd(ecs::Entity entity) override {}
    void onRemove(ecs::Entity entity) override;
    void onEvent(DeathEvent, ecs::Entity entity) override;
};

}  // namespace whal

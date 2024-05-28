#pragma once

#include "Systems/Event.h"
#include "Systems/System.h"
#include "whalECS/src/ECS.h"

namespace whal {

struct Children;
struct Attach;
struct Follow;
struct Velocity;
struct Transform2D;

class EntityChildSystem : public ecs::ISystem<Children> {
public:
    EntityChildSystem();
    static std::shared_ptr<EntityChildSystem> instance() {
        static std::shared_ptr<EntityChildSystem> instance_ = System::ecs->registerSystem<EntityChildSystem>();
        return instance_;
    }
    void onRemove(ecs::Entity entity) override;

private:
    EventListener<ecs::Entity> mEntityDeathListener;
};

class AttachSystem : public ecs::ISystem<Attach, Transform2D> {
public:
    void update() override;
    void onAdd(ecs::Entity entity) override;
};

class FollowSystem : public ecs::ISystem<Follow, Velocity, Transform2D> {
public:
    FollowSystem();
    void update() override;
    void onRemove(ecs::Entity entity) override;

private:
    EventListener<ecs::Entity> mEntityDeathListener;
};

void unfollowEntity(ecs::Entity killedEntity);
void removeEntityFromChildList(ecs::Entity entity);

}  // namespace whal

#pragma once

#include "Systems/Event.h"
#include "whalECS/src/ECS.h"

namespace whal {

struct Children;
struct Follow;
struct Velocity;
struct Transform;

class EntityChildSystem : public ecs::ISystem<Children> {
public:
    EntityChildSystem();
    static std::shared_ptr<EntityChildSystem> instance() {
        static std::shared_ptr<EntityChildSystem> instance_ = ecs::ECS::getInstance().registerSystem<EntityChildSystem>();
        return instance_;
    }
    void onRemove(ecs::Entity entity) override;

private:
    EventListener<ecs::Entity> mEntityDeathListener;
};

class FollowSystem : public ecs::ISystem<Follow, Velocity, Transform> {
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

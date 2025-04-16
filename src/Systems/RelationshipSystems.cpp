#include "RelationshipSystems.h"

namespace whal {

void FollowSystem::update() {
    // i nuked this because i wrote it when this engine was a baby and it didn't even work
    // for (auto [entityid, entity] : getEntities()) {
    //     Transform trans = entity.get<Transform>();
    //     auto& follow = entity.get<Follow>();
    //     if (!follow.isTargetInitialized) {
    //         follow.initTarget(entity);
    //     }
    // }
}

void FollowSystem::onRemove(ecs::Entity entity) {
    // reset any lingering effects on velocity
    // if (entity.has<Velocity>()) {
    //     entity.set(Velocity());
    // }
}

// if the target of an entity's Follow component dies, remove the follow component.
void FollowSystem::onEvent(evt::EntityDestroyed, ecs::Entity killedEntity) {
    // std::vector<ecs::Entity> toRemove;
    // for (auto& [entityid, entity] : FollowSystem::getEntities()) {
    //     if (entity.get<Follow>().targetEntityID == killedEntity.id()) {
    //         toRemove.push_back(entity);
    //     }
    // }
    //
    // for (auto entity : toRemove) {
    //     entity.remove<Follow>();
    // }
}

}  // namespace whal

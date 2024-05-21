#include "TriggerSystem.h"
#include "ECS/Collision.h"
#include "ECS/TriggerZone.h"

namespace whal {

void TriggerSystem::update() {
    for (auto [entityid, entity] : getEntitiesRef()) {
        std::vector<ecs::Entity> newInsideList;
        auto& trigger = entity.get<TriggerZone>();

        for (auto [actorid, actor] : MovableActorTracker::getEntitiesRef()) {
            bool wasInside = std::find(trigger.insideEntities.begin(), trigger.insideEntities.end(), actor) != trigger.insideEntities.end();

            if (actor.get<ActorCollider>().getCollider().isOverlapping(trigger)) {
                newInsideList.push_back(actor);
                if (!wasInside && trigger.onTriggerEnter != nullptr) {
                    trigger.onTriggerEnter(entity, actor);
                }
            } else if (wasInside && trigger.onTriggerExit != nullptr) {
                trigger.onTriggerExit(entity, actor);
            }
        }

        trigger.insideEntities = std::move(newInsideList);
    }
}

}  // namespace whal

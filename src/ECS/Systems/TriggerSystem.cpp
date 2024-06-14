#include "TriggerSystem.h"

#include "ECS/Collision.h"
#include "ECS/TriggerZone.h"

namespace whal {

void TriggerSystem::update() {
    for (auto [entityid, entity] : getEntitiesRef()) {
        std::vector<ecs::Entity> newInsideList;
        auto& trigger = entity.get<Trigger>();

        // TODO instead of tracking movable actors, use Layer Matrix
        // can add multiple types of triggers depending on what I want them to interact with (e.g. just Actors vs Everything)
        for (auto [actorid, actor] : MovableActorTracker::getEntitiesRef()) {
            bool wasInside = whal_find(trigger.insideEntities.begin(), trigger.insideEntities.end(), actor) != trigger.insideEntities.end();

            if (trigger.shape.isOverlapping(actor.get<Collider>().getShape())) {
                newInsideList.push_back(actor);
                if (!wasInside && trigger.onTriggerEnter != nullptr) {
                    trigger.onTriggerEnter(entity, actor);
                } else if (wasInside && trigger.onTriggerStay != nullptr) {
                    trigger.onTriggerStay(entity, actor);
                }
            } else if (wasInside && trigger.onTriggerExit != nullptr) {
                trigger.onTriggerExit(entity, actor);
            }
        }

        trigger.insideEntities = std::move(newInsideList);
    }
}

}  // namespace whal

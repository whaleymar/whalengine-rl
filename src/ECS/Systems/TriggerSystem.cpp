#include "TriggerSystem.h"

#include "ECS/Collision.h"
#include "ECS/TriggerZone.h"
#include "Physics/CollisionLayer.h"

namespace whal {

void TriggerSystem::update() {
    for (auto [entityid, entity] : getEntitiesRef()) {
        std::vector<ecs::Entity> newInsideList;
        auto& trigger = entity.get<Trigger>();

        for (auto [otherid, other] : MovableColliders::getEntitiesRef()) {
            auto collider = other.get<Collider>();
            if (!LAYER_MATRIX.isOn(trigger.layer, collider.getCollisionLayer())) {
                continue;
            }
            bool wasInside = whal_find(trigger.insideEntities.begin(), trigger.insideEntities.end(), other) != trigger.insideEntities.end();

            if (trigger.shape.isOverlapping(collider.getShape())) {
                newInsideList.push_back(other);
                if (!wasInside && trigger.onTriggerEnter != nullptr) {
                    trigger.onTriggerEnter(entity, other);
                } else if (wasInside && trigger.onTriggerStay != nullptr) {
                    trigger.onTriggerStay(entity, other);
                }
            } else if (wasInside && trigger.onTriggerExit != nullptr) {
                trigger.onTriggerExit(entity, other);
            }
        }

        trigger.insideEntities = std::move(newInsideList);
    }
}

}  // namespace whal

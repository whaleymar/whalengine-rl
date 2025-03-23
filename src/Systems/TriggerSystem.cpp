#include "TriggerSystem.h"

#include "Components/Collider.h"
#include "Components/Transform.h"
#include "Components/Trigger.h"
#include "Physics/CollisionLayer.h"
#include "Systems/ColliderSystem.h"

namespace whal {

void TriggerSystem::update() {
    for (const auto& [entityid, entity] : getEntities()) {
        std::vector<ecs::Entity> newInsideList;
        auto& trigger = entity.get<Trigger>();
        const auto trans = entity.get<Transform>();

        // update trigger zone w/ transform
        trigger.shape.setPosition(trans, trigger.offset);

        for (auto other : ColliderSystem::query(trigger.shape.getBoundingBox())) {
            auto collider = other.get<Collider>();

            if ((trigger.layerMask & collider.getLayerMask()) == 0) {
                continue;
            }
            auto it = whal_find(trigger.insideEntities.begin(), trigger.insideEntities.end(), other);
            const bool wasInside = it != trigger.insideEntities.end();
            if (wasInside)
                trigger.insideEntities.erase(it);  // so I can run onTriggerExit on entities outside BB

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

        // any entities remaining in insideEntities have exited the trigger zone & aren't in the shape's bounding box.
        if (trigger.onTriggerExit != nullptr) {
            for (auto other : trigger.insideEntities) {
                trigger.onTriggerExit(entity, other);
            }
        }

        trigger.insideEntities = std::move(newInsideList);
    }
}

}  // namespace whal

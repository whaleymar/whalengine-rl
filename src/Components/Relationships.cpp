#include "Relationships.h"

#include "whalECS/src/ECS.h"

namespace whal {

Follow::Follow(ecs::Entity target_) : targetEntityID(target_.id()) {}

void Follow::initTarget(ecs::Entity self) {
    // isTargetInitialized = true;
    // ecs::Entity targetEntity(targetEntityID);
    // currentTarget = targetEntity.get<Transform>().positionPx;
}

}  // namespace whal

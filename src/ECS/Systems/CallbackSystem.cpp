#include "CallbackSystem.h"

#include "ECS/Callback.h"

namespace whal {

void OnFrameEndSystem::fixedUpdate() {
    for (auto [entityid, entity] : getEntitiesCopy()) {
        // not bothering with a null check

        const auto onFrameEnd = entity.get<OnFrameEnd>();
        onFrameEnd.callback(entity);

        if (onFrameEnd.removeSelf) {
            entity.remove<OnFrameEnd>();
        }
    }
}

}  // namespace whal

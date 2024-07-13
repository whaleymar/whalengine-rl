#include "CallbackSystem.h"

#include "Components/Callback.h"

namespace whal {

void OnFrameEndSystem::update() {
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

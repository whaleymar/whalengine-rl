#include "TagTrackers.h"

#include "ECS/Transform.h"

namespace whal {

std::optional<ecs::Entity> getCamera() {
    if (CameraSystem::instance()->getEntitiesRef().empty()) {
        return std::nullopt;
    }
    return CameraSystem::instance()->first();
}

Vector2i getCameraPosition() {
    static Vector2i lastPos;
    auto eOpt = getCamera();
    if (eOpt) {
        auto lastPosOpt = eOpt.value().tryGet<Transform>();
        if (lastPosOpt) {
            lastPos = lastPosOpt.value()->position;
        }
    }
    return lastPos;
}

}  // namespace whal

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
        auto lastPosOpt = eOpt.value().tryGet<Transform2D>();
        if (lastPosOpt) {
            lastPos = lastPosOpt.value()->position;
        }
    }
    return lastPos;
}

void setCameraPosition(Vector2i pos) {
    auto eOpt = getCamera();
    if (eOpt) {
        eOpt.value().set(Transform2D(pos));
    }
}

}  // namespace whal

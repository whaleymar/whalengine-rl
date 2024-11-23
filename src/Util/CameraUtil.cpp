#include "CameraUtil.h"

#include "Components/Relationships.h"
#include "Systems/CameraSystem.h"

namespace whal {

Corrade::Containers::Optional<ecs::Entity> getCamera() {
    if (World.getSystem<CameraSystem>()->getEntitiesMutable().empty()) {
        return Corrade::Containers::NullOpt;
    }
    return World.getSystem<CameraSystem>()->first();
}

Vector2i getCameraPosition() {
    static Vector2i lastPos;
    auto eOpt = getCamera();
    if (eOpt) {
        lastPos = eOpt->get<Transform>().position;
    }
    return lastPos;
}

Vector2f getCameraPositionPrecise() {
    static Vector2f lastPos;
    auto eOpt = getCamera();
    if (eOpt) {
        lastPos = eOpt->get<PrecisePosition>().position;
    }

    return lastPos;
}

void setCameraTarget(ecs::Entity target) {
    if (auto cameraOpt = getCamera(); cameraOpt) {
        auto camera = *cameraOpt;
        if (camera.has<Follow>()) {
            auto& follow = camera.get<Follow>();
            follow.targetEntityID = target.id();
        } else {
            camera.add(Follow(target));
        }
    }
}

void setCameraPosition(Vector2i pos) {
    auto eOpt = getCamera();
    if (eOpt) {
        eOpt->set(Transform(pos));
    }
}

}  // namespace whal

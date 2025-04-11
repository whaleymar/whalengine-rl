#include "CameraUtil.h"

#include "Components/Relationships.h"
#include "Components/Tags.h"
#include "Components/Transform.h"
#include "Systems/CameraSystem.h"

namespace whal {

static ecs::Entity createCamera() {
    auto camera = World.entity("Camera", false);
    if (!camera.isValid()) {
        return camera;
    }
    auto _ = ecs::DeferActivate(camera);

    camera.add<Camera>();
    camera.add<AudioListener>();
    camera.add<IgnoreTimeModifiers>();

    return camera;
}

ecs::Entity getCamera() {
    if (World.getSystem<CameraSystem>()->getEntities().empty()) {
        return createCamera();
    }
    return World.getSystem<CameraSystem>()->first();
}

Vector2i getCameraPosition() {
    return getCamera().get<Transform>().positionPx;
}

Vector2f getCameraPositionPrecise() {
    return getCamera().get<Transform>().position;
}

void setCameraTarget(ecs::Entity target) {
    ecs::Entity camera = getCamera();
    if (camera.has<Follow>()) {
        auto& follow = camera.get<Follow>();
        follow.targetEntityID = target.id();
    } else {
        camera.add(Follow(target));
    }
}

void setCameraPosition(Vector2i pos) {
    getCamera().set(Transform::world(pos.x, pos.y));
}

}  // namespace whal

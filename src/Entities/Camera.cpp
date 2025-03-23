#include "Camera.h"

#include "ECS.h"

#include "Components/Camera.h"
#include "Components/Tags.h"
#include "Components/Transform.h"

#include "Sys/System.h"

namespace whal {

ecs::Entity createCamera(Transform trans) {
    auto camera = World.entity("Camera", false);
    if (!camera.isValid()) {
        return camera;
    }
    auto _ = ecs::DeferActivate(camera);

    camera.set(trans);
    camera.add<Camera>();
    camera.add<AudioListener>();
    camera.add<IgnoreTimeModifiers>();

    return camera;
}

}  // namespace whal

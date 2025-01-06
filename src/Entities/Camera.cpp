#include "Camera.h"

#include "whalECS/src/ECS.h"

#include "Components/Callback.h"
#include "Components/Camera.h"
#include "Components/Name.h"
#include "Components/RailsControl.h"
#include "Components/Relationships.h"
#include "Components/Tags.h"
#include "Components/Transform.h"

#include "Sys/System.h"

// #include "Components/PlayerControl.h"
// #include "Components/Velocity.h"

namespace whal {

ecs::Entity createCamera(Transform trans) {
    auto camera = World.entity(false);
    if (!camera.isValid()) {
        return camera;
    }
    auto _ = ecs::DeferActivate(camera);

    camera.set(trans);
    camera.add(Name("Camera"));
    camera.add<Camera>();
    camera.add<AudioListener>();
    camera.add<IgnoreTimeModifiers>();
    // camera.add<PlayerControl>();
    // camera.add<Velocity>();

    return camera;
}

}  // namespace whal

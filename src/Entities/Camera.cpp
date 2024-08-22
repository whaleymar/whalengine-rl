#include "Camera.h"

#include "whalECS/src/ECS.h"

#include "Components/Callback.h"
#include "Components/Name.h"
#include "Components/RailsControl.h"
#include "Components/Relationships.h"
#include "Components/Tags.h"
#include "Components/Transform.h"

#include "Sys/System.h"

namespace whal {

Expected<ecs::Entity> createCamera(Transform2D trans) {
    auto expected = System::world.entity(false);
    if (!expected.isExpected()) {
        return expected;
    }
    auto _ = ecs::DeferActivate(expected.value());

    auto camera = expected.value();
    camera.add(trans);
    camera.add(PrecisePosition::fromTrans(trans));
    camera.add(Name("Camera"));
    camera.add<Camera>();
    camera.add<AudioListener>();

    return camera;
}

}  // namespace whal

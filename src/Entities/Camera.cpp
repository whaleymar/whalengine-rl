#include "Camera.h"

#include "whalECS/src/ECS.h"

#include "Components/Callback.h"
#include "Components/Name.h"
#include "Components/RailsControl.h"
#include "Components/Relationships.h"
#include "Components/Tags.h"
#include "Components/Transform.h"
#include "Components/Velocity.h"

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
    camera.add<Velocity>();
    camera.add(Name("Camera"));
    camera.add<Camera>();
    camera.add<AudioListener>();

    return camera;
}

void onCameraAtDestination(ecs::Entity cameraEntity, RailsControl& rails) {
    auto frameEndCallback = [](ecs::Entity entity) {
        entity.remove<RailsControl>();
    };  // don't call this immediately cause it will mutate the Rails system's entity list while it's iterating
    cameraEntity.add(OnFrameEnd(frameEndCallback));
    cameraEntity.add<Velocity>();  // railscontrol removed it
    System::dt.setMultiplier(1.0);
}

RailsControl createCameraMoveController(Vector2i currentPosition, Vector2i nextPosition) {
    return RailsControl(520,
                        {
                            {currentPosition, Ease::Linear},
                            {nextPosition, Ease::OutCubic},
                        },
                        0, RailsControl::CycleBehavior::AUTOMATIC_LOOP, &onCameraAtDestination);
}

}  // namespace whal

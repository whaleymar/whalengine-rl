#include "Camera.h"

#include "whalECS/src/ECS.h"

#include "ECS/Callback.h"
#include "ECS/Name.h"
#include "ECS/RailsControl.h"
#include "ECS/Relationships.h"
#include "ECS/Tags.h"
#include "ECS/Transform.h"
#include "ECS/Velocity.h"
#include "Systems/System.h"

namespace whal {

Expected<ecs::Entity> createCamera(ecs::Entity target) {
    auto& ecs = ecs::ECS::getInstance();

    auto expected = ecs.entity();
    if (!expected.isExpected()) {
        return expected;
    }

    auto camera = expected.value();
    camera.add(target.get<Transform2D>());
    camera.add(Follow(target));
    // camera.add<PlayerControlFree>();
    camera.add<Velocity>();
    camera.add(Name("Camera"));
    camera.add<Camera>();

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
                            {currentPosition, RailsControl::Movement::LINEAR},
                            {nextPosition, RailsControl::Movement::EASEO_CUBE},
                        },
                        0, true, &onCameraAtDestination);
}

}  // namespace whal

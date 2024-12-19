#include "CameraSystem.h"

#include "Components/Tags.h"
#include "Map/Level.h"
#include "Sys/Tween.h"

namespace whal {

void CameraSystem::onEvent(evt::EnteredLevel, ecs::Entity player, ActiveLevel& activeLevel) {
    auto camera = first();
    if (activeLevel.cameraFollow) {
        Follow follow = *activeLevel.cameraFollow;
        follow.targetEntityID = player.id();
        if (camera.has<Follow>()) {
            camera.set(follow);
        } else {
            camera.add(follow);
        }
        return;
    } else {
        Vector2f focalPoint = activeLevel.cameraFocalPoint.as<f32>();
        if (camera.has<Follow>()) {
            camera.remove<Follow>();
        }

        if (camera.get<Transform>().position == focalPoint) {
            return;
        }

        Schedule.tween(camera, focalPoint.as<f32>(), 0.5, &Transform::position, &Transform::setPosition)
            .setTransition(Ease::InOutQuad)
            .asIgnoreSlowdown()
            .setOnEnd([](ecs::Entity self, const Tween<Vector2f>&) { Time.setMultiplier(1.0); });
        Time.setMultiplier(0.0);
        return;
    }
}

void CameraSystem::onEvent(evt::Pause, bool isPaused) {
    auto camera = first();
    if (isPaused) {
        camera.remove<IgnoreTimeModifiers>();
    } else {
        camera.add<IgnoreTimeModifiers>();
    }
}

}  // namespace whal

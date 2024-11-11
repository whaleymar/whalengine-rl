#include "TagSystems.h"

#include "Components/RailsControl.h"
#include "Components/Tags.h"
#include "Components/Transform.h"

#include "Entities/Camera.h"
#include "Map/Level.h"

#include "Sys/System.h"
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
        Vector2i focalPoint = activeLevel.cameraFocalPoint;
        if (camera.has<Follow>()) {
            camera.remove<Follow>();
        }

        if (camera.get<Transform2D>().position == focalPoint) {
            return;
        }

        TweenManager::create(camera, focalPoint.as<f32>(), 0.5, [](ecs::Entity self) -> auto& { return self.get<PrecisePosition>().position; })
            .setTransition(Ease::InOutQuad)
            .asIgnoreSlowdown()
            .setOnUpdate([](ecs::Entity self, const TweenVec2f&) { self.get<Transform2D>().position = self.get<PrecisePosition>().position.round(); })

            .setOnEnd([](ecs::Entity self, const TweenVec2f&) { System::time.setMultiplier(1.0); });
        System::time.setMultiplier(0.0);
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

void AudioListenerSystem::update() {
    if (getEntitiesMutable().empty()) {
        return;
    }
    auto listenerEntity = first();
    System::audio.setListenerPosition(listenerEntity.get<Transform2D>().position);
}

}  // namespace whal

#include "TagTrackers.h"

#include "Components/RailsControl.h"
#include "Components/Tags.h"
#include "Components/Transform.h"

#include "Entities/Camera.h"
#include "Map/Level.h"

#include "Sys/System.h"
#include "Sys/Tween.h"

namespace whal {

void CameraSystem::onEvent(EnteredLevelEvent, ecs::Entity player, ActiveLevel& activeLevel) {
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

void CameraSystem::onEvent(PauseEvent, bool isPaused) {
    auto camera = first();
    if (isPaused) {
        camera.remove<IgnoreTimeModifiers>();
    } else {
        camera.add<IgnoreTimeModifiers>();
    }
}

Corrade::Containers::Optional<ecs::Entity> getCamera() {
    if (System::world.getSystem<CameraSystem>()->getEntitiesMutable().empty()) {
        return Corrade::Containers::NullOpt;
    }
    return System::world.getSystem<CameraSystem>()->first();
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

Vector2i getCameraPosition() {
    static Vector2i lastPos;
    auto eOpt = getCamera();
    if (eOpt) {
        lastPos = eOpt->get<Transform2D>().position;
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

void setCameraPosition(Vector2i pos) {
    auto eOpt = getCamera();
    if (eOpt) {
        eOpt->set(Transform2D(pos));
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

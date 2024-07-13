#include "TagTrackers.h"

#include "Components/RailsControl.h"
#include "Components/Transform.h"
#include "Entities/Camera.h"
#include "Map/Level.h"
#include "Sys/System.h"

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
        camera.add(createCameraMoveController(camera.get<Transform2D>().position, focalPoint));
        System::dt.setMultiplier(0.0);
        return;
    }
}

Corrade::Containers::Optional<ecs::Entity> getCamera() {
    if (System::world->getSystem<CameraSystem>()->getEntitiesRef().empty()) {
        return Corrade::Containers::NullOpt;
    }
    return System::world->getSystem<CameraSystem>()->first();
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

    // getting weird floating point precision errors when camera position is very close to X.5
    // y
    f32 whole, fractional;
    fractional = std::modf(lastPos.y(), &whole);
    if (fractional > 0.47 && fractional < 0.53) {
        fractional = 0.46;
        lastPos.e[1] = (whole + fractional);
    } else if (fractional < -0.47 && fractional > -0.53) {
        fractional = -0.46;
        lastPos.e[1] = (whole + fractional);
    }

    // x
    fractional = std::modf(lastPos.x(), &whole);
    if (fractional > 0.47 && fractional < 0.53) {
        fractional = 0.46;
        lastPos.e[0] = (whole + fractional);
    } else if (fractional < -0.47 && fractional > -0.53) {
        fractional = -0.46;
        lastPos.e[0] = (whole + fractional);
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
    if (getEntitiesRef().empty()) {
        return;
    }
    auto listenerEntity = first();
    System::audio.setListenerPosition(listenerEntity.get<Transform2D>().position);
}

}  // namespace whal

#include "TagTrackers.h"

#include "ECS/Transform.h"
#include "Systems/System.h"

namespace whal {

Corrade::Containers::Optional<ecs::Entity> getCamera() {
    if (System::world->getSystem<CameraSystem>()->getEntitiesRef().empty()) {
        return Corrade::Containers::NullOpt;
    }
    return System::world->getSystem<CameraSystem>()->first();
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

void AudioListenerSystem::fixedUpdate() {
    if (getEntitiesRef().empty()) {
        return;
    }
    auto listenerEntity = first();
    System::audio.setListenerPosition(listenerEntity.get<Transform2D>().position);
}

}  // namespace whal

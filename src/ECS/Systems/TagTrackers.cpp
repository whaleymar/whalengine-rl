#include "TagTrackers.h"

#include "ECS/Transform.h"
#include "Systems/System.h"

namespace whal {

PlayerSystem* PlayerSystem::instance() {
    static auto instance_ = System::ecs->registerSystem<PlayerSystem>();
    return instance_;
}

CameraSystem* CameraSystem::instance() {
    static auto instance_ = System::ecs->registerSystem<CameraSystem>(ecs::SystemManager::UniqueEntity);
    return instance_;
}

AudioListenerSystem* AudioListenerSystem::instance() {
    static auto instance_ = System::ecs->registerSystem<AudioListenerSystem>(ecs::SystemManager::UniqueEntity);
    return instance_;
}

Corrade::Containers::Optional<ecs::Entity> getCamera() {
    if (CameraSystem::instance()->getEntitiesRef().empty()) {
        return Corrade::Containers::NullOpt;
    }
    return CameraSystem::instance()->first();
}

Vector2i getCameraPosition() {
    static Vector2i lastPos;
    auto eOpt = getCamera();
    if (eOpt) {
        lastPos = eOpt->get<Transform2D>().position;
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

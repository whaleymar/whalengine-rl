#include "TagTrackers.h"

#include "ECS/Transform.h"
#include "Systems/System.h"

namespace whal {

std::shared_ptr<PlayerSystem> PlayerSystem::instance() {
    static std::shared_ptr<PlayerSystem> instance_ = System::ecs->registerSystem<PlayerSystem>();
    return instance_;
}

std::shared_ptr<CameraSystem> CameraSystem::instance() {
    static std::shared_ptr<CameraSystem> instance_ = System::ecs->registerSystem<CameraSystem>(ecs::SystemManager::UniqueEntity);
    return instance_;
}

std::shared_ptr<AudioListenerSystem> AudioListenerSystem::instance() {
    static std::shared_ptr<AudioListenerSystem> instance_ = System::ecs->registerSystem<AudioListenerSystem>(ecs::SystemManager::UniqueEntity);
    return instance_;
}

std::optional<ecs::Entity> getCamera() {
    if (CameraSystem::instance()->getEntitiesRef().empty()) {
        return std::nullopt;
    }
    return CameraSystem::instance()->first();
}

Vector2i getCameraPosition() {
    static Vector2i lastPos;
    auto eOpt = getCamera();
    if (eOpt) {
        lastPos = eOpt.value().get<Transform2D>().position;
    }
    return lastPos;
}

void setCameraPosition(Vector2i pos) {
    auto eOpt = getCamera();
    if (eOpt) {
        eOpt.value().set(Transform2D(pos));
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

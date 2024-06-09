#pragma once

#include "Systems/System.h"
#include "Util/Vector.h"
#include "whalECS/src/ECS.h"

namespace whal {

struct Player;
struct Camera;
struct AudioListener;
struct Transform2D;

class PlayerSystem : public ecs::ISystem<Player> {
public:
    static std::shared_ptr<PlayerSystem> instance() {
        static std::shared_ptr<PlayerSystem> instance_ = System::ecs->registerSystem<PlayerSystem>();
        return instance_;
    }
};

class CameraSystem : public ecs::ISystem<Camera, Transform2D> {
public:
    static std::shared_ptr<CameraSystem> instance() {
        static std::shared_ptr<CameraSystem> instance_ = System::ecs->registerSystem<CameraSystem>(ecs::SystemManager::UniqueEntity);
        return instance_;
    }
};

std::optional<ecs::Entity> getCamera();
Vector2i getCameraPosition();
void setCameraPosition(Vector2i pos);

class AudioListenerSystem : public ecs::ISystem<AudioListener, Transform2D> {
public:
    static std::shared_ptr<AudioListenerSystem> instance() {
        static std::shared_ptr<AudioListenerSystem> instance_ = System::ecs->registerSystem<AudioListenerSystem>(ecs::SystemManager::UniqueEntity);
        return instance_;
    }

    void update() override;
};

}  // namespace whal

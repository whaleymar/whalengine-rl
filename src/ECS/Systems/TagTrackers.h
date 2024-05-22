#pragma once

#include "Systems/System.h"
#include "Util/Vector.h"
#include "whalECS/src/ECS.h"

namespace whal {

struct Player;
struct Camera;

class PlayerSystem : public ecs::ISystem<Player> {
public:
    static std::shared_ptr<PlayerSystem> instance() {
        static std::shared_ptr<PlayerSystem> instance_ = System::ecs->registerSystem<PlayerSystem>();
        return instance_;
    }
};

class CameraSystem : public ecs::ISystem<Camera> {
public:
    static std::shared_ptr<CameraSystem> instance() {
        static std::shared_ptr<CameraSystem> instance_ = System::ecs->registerSystem<CameraSystem>();
        return instance_;
    }
};

std::optional<ecs::Entity> getCamera();
Vector2i getCameraPosition();

}  // namespace whal

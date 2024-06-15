#pragma once

#include "Util/Vector.h"
#include "whalECS/src/ECS.h"

namespace whal {

struct Player;
struct Camera;
struct AudioListener;
struct Transform2D;

class PlayerSystem : public ecs::ISystem<Player> {
public:
    static PlayerSystem* instance();
};

class CameraSystem : public ecs::ISystem<Camera, Transform2D> {
public:
    static CameraSystem* instance();
};

std::optional<ecs::Entity> getCamera();
Vector2i getCameraPosition();
void setCameraPosition(Vector2i pos);

class AudioListenerSystem : public ecs::ISystem<AudioListener, Transform2D> {
public:
    static AudioListenerSystem* instance();

    void update() override;
};

}  // namespace whal

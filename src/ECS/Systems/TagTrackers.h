#pragma once

#include "Util/Vector.h"
#include "whalECS/src/ECS.h"

namespace whal {

struct Player;
struct Camera;
struct AudioListener;
struct Transform2D;

class PlayerSystem : public ecs::ISystem<Player> {};

class CameraSystem : public ecs::ISystem<Camera, Transform2D>, public ecs::AttrUniqueEntity {};

Corrade::Containers::Optional<ecs::Entity> getCamera();
Vector2i getCameraPosition();
Vector2f getCameraPositionPrecise();
void setCameraPosition(Vector2i pos);

class AudioListenerSystem : public ecs::ISystem<AudioListener, Transform2D>,
                            public ecs::IFixedUpdate,
                            public ecs::AttrUniqueEntity,
                            public ecs::AttrUpdateDuringPause {
public:
    void fixedUpdate() override;
};

}  // namespace whal

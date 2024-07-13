#pragma once

#include "Events/Events.h"
#include "Sys/System.h"
#include "Util/Vector.h"
#include "whalECS/src/ECS.h"

namespace whal {

struct Player;
struct Camera;
struct AudioListener;
struct Transform2D;

class PlayerSystem : public ecs::ISystem<Player> {};

class CameraSystem : public ecs::ISystem<Camera, Transform2D>,
                     public ecs::AttrUniqueEntity,
                     public IListen<EnteredLevelEvent, true, ecs::Entity, ActiveLevel&> {
public:
    void onEvent(EnteredLevelEvent, ecs::Entity player, ActiveLevel& activeLevel);
};

Corrade::Containers::Optional<ecs::Entity> getCamera();
Vector2i getCameraPosition();
Vector2f getCameraPositionPrecise();
void setCameraPosition(Vector2i pos);
void setCameraTarget(ecs::Entity target);

class AudioListenerSystem : public ecs::ISystem<AudioListener, Transform2D>,
                            public ecs::IUpdate,
                            public ecs::AttrUniqueEntity,
                            public ecs::AttrUpdateDuringPause {
public:
    void update() override;
};

}  // namespace whal

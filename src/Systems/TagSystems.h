#pragma once

#include "Events/Events.h"
#include "Sys/IListen.h"
#include "whalECS/src/ECS.h"

namespace whal {

struct Player;
struct Camera;
struct AudioListener;
struct Transform2D;

class PlayerSystem : public ecs::ISystem<Player> {};

class CameraSystem : public ecs::ISystem<Camera, Transform2D>,
                     public ecs::AttrUniqueEntity,
                     public IListen<evt::EnteredLevel, true, ecs::Entity, ActiveLevel&>,
                     public IListen<evt::Pause, true, bool> {
public:
    void onEvent(evt::EnteredLevel, ecs::Entity player, ActiveLevel& activeLevel) override;
    void onEvent(evt::Pause, bool isPaused) override;
};

class AudioListenerSystem : public ecs::ISystem<AudioListener, Transform2D>,
                            public ecs::IUpdate,
                            public ecs::AttrUniqueEntity,
                            public ecs::AttrUpdateDuringPause {
public:
    void update() override;
};

}  // namespace whal

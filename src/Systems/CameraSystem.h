#pragma once

#include "Events/Events.h"
#include "Sys/IListen.h"
#include "whalECS/src/ECS.h"

namespace whal {

struct Camera;
struct Transform2D;

class CameraSystem : public ecs::ISystem<Camera, Transform2D>,
                     public ecs::AttrUniqueEntity,
                     public IListen<evt::EnteredLevel, true, ecs::Entity, ActiveLevel&>,
                     public IListen<evt::Pause, true, bool> {
public:
    void onEvent(evt::EnteredLevel, ecs::Entity player, ActiveLevel& activeLevel) override;
    void onEvent(evt::Pause, bool isPaused) override;
};

}  // namespace whal

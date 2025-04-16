#pragma once

#include "Components/Camera.h"
#include "Components/Transform.h"
#include "Events/Events.h"
#include "Sys/IListen.h"
#include "whalECS/src/ECS.h"

namespace whal {

class CameraSystem : public ecs::ISystem<Camera, Transform>,
                     public ecs::AttrUniqueEntity,
                     public IListen<evt::EnteredLevel, true, ecs::Entity, TileMap&>,
                     public IListen<evt::Pause, true, bool> {
public:
    void onEvent(evt::EnteredLevel, ecs::Entity player, TileMap& activeLevel) override;
    void onEvent(evt::Pause, bool isPaused) override;
};

}  // namespace whal

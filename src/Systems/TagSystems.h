#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct Player;
struct AudioListener;
struct Transform2D;

class PlayerSystem : public ecs::ISystem<Player> {};

class AudioListenerSystem : public ecs::ISystem<AudioListener, Transform2D>,
                            public ecs::IUpdate,
                            public ecs::AttrUniqueEntity,
                            public ecs::AttrUpdateDuringPause {
public:
    void update() override;
};

}  // namespace whal

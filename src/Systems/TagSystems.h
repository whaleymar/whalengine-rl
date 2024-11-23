#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct Player;
struct AudioListener;
struct Transform;

class PlayerSystem : public ecs::ISystem<Player> {};

class AudioListenerSystem : public ecs::ISystem<AudioListener, Transform>,
                            public ecs::IUpdate,
                            public ecs::AttrUniqueEntity,
                            public ecs::AttrUpdateDuringPause {
public:
    void update() override;
};

}  // namespace whal

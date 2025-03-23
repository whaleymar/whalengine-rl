#pragma once

#include "Components/Tags.h"
#include "Components/Transform.h"
#include "whalECS/src/ECS.h"

namespace whal {

class PlayerSystem : public ecs::ISystem<Player> {};

class AudioListenerSystem : public ecs::ISystem<AudioListener, Transform>,
                            public ecs::IUpdate,
                            public ecs::AttrUniqueEntity,
                            public ecs::AttrUpdateDuringPause {
public:
    void update() override;
};

}  // namespace whal

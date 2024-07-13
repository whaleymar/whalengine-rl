#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct OnFrameEnd;

class OnFrameEndSystem : public ecs::ISystem<OnFrameEnd>, public ecs::IUpdate {
public:
    void update() override;
};

}  // namespace whal

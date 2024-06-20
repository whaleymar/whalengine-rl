#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct OnFrameEnd;

class OnFrameEndSystem : public ecs::ISystem<OnFrameEnd>, public ecs::IFixedUpdate {
public:
    void fixedUpdate() override;
};

}  // namespace whal

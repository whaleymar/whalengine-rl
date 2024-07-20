#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct OnFrameEnd;
struct CustomUpdate;

class OnFrameEndSystem : public ecs::ISystem<OnFrameEnd>, public ecs::IUpdate {
public:
    void update() override;
};

class CustomUpdateSystem : public ecs::ISystem<CustomUpdate>, public ecs::IUpdate {
public:
    void update() override;
};

}  // namespace whal

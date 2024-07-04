#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct Transform2D;
struct Trigger;

class TriggerSystem : public ecs::ISystem<Transform2D, Trigger>, public ecs::IUpdate {
public:
    void update() override;
};

}  // namespace whal

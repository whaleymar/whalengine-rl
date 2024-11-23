#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct Transform;
struct Trigger;

class TriggerSystem : public ecs::ISystem<Transform, Trigger>, public ecs::IUpdate {
public:
    void update() override;
};

}  // namespace whal

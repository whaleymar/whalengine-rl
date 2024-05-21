#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct RailsControl;
struct Velocity;
struct Transform;

class RailsSystem : public ecs::ISystem<RailsControl, Transform> {
public:
    void update() override;
};

}  // namespace whal

#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct RailsControl;
struct Velocity;
struct Transform2D;

class RailsSystem : public ecs::ISystem<RailsControl, Transform2D> {
public:
    void update() override;
};

}  // namespace whal

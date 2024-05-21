#pragma once

#include "whalECS/src/ECS.h"

#include "Util/Types.h"

namespace whal {

struct Transform;
struct Velocity;

inline constexpr f32 TERMINAL_VELOCITY_Y = -160;

class PhysicsSystem : public ecs::ISystem<Transform, Velocity> {
public:
    void update() override;
};

}  // namespace whal

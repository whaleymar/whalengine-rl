#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct Transform2D;
struct ParticleEmitter;

class ParticleEmitterSystem : public ecs::ISystem<ParticleEmitter, Transform2D>, public ecs::IUpdate {
public:
    void update() override;
};

}  // namespace whal

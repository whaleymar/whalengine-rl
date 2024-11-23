#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct Transform;
struct ParticleEmitter;

class ParticleEmitterSystem : public ecs::ISystem<ParticleEmitter, Transform>, public ecs::IUpdate {
public:
    void update() override;
};

}  // namespace whal

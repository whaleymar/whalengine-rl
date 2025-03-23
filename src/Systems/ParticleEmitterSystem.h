#pragma once

#include "Components/ParticleEmitter.h"
#include "Components/Transform.h"
#include "whalECS/src/ECS.h"

namespace whal {

class ParticleEmitterSystem : public ecs::ISystem<ParticleEmitter, Transform>, public ecs::IUpdate {
public:
    void update() override;
};

}  // namespace whal

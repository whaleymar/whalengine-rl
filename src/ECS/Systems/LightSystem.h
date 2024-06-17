#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct PointLight;
struct Transform2D;

class PointLightSystem : public ecs::ISystem<Transform2D, PointLight> {
public:
    void update() override;
};

}  // namespace whal

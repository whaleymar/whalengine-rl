#pragma once

#include "whalECS/src/ECS.h"

typedef struct Shader Shader;

namespace whal {

struct PointLight;
struct Radiance;
struct Transform2D;

class PointLightSystem : public ecs::ISystem<Transform2D, PointLight> {
public:
    PointLightSystem();
    void update();

private:
    int mPositionUniform;
};

class RadianceLightSystem : public ecs::ISystem<Transform2D, Radiance> {
public:
    RadianceLightSystem();
    void update();

private:
    int mPositionUniform;
};

}  // namespace whal

#pragma once

#include "whalECS/src/ECS.h"

typedef struct Shader Shader;

namespace whal {

struct PointLight;
struct BoxLight;
struct Radiance;
struct Transform2D;

void drawLights();

class PointLightSystem : public ecs::ISystem<Transform2D, PointLight> {
public:
    PointLightSystem();
    void update();

private:
    int mPositionUniform;
};

class BoxLightSystem : public ecs::ISystem<Transform2D, BoxLight> {
public:
    BoxLightSystem();
    void update();

private:
    int mPositionUniform;
    int mHalflenUniform;
    int mRadiusUniform;
};

class RadianceLightSystem : public ecs::ISystem<Transform2D, Radiance> {
public:
    RadianceLightSystem();
    void update();

private:
    int mPositionUniform;
};

}  // namespace whal

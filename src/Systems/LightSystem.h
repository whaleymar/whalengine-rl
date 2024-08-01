#pragma once

#include "whalECS/src/ECS.h"

typedef struct Shader Shader;
typedef struct Camera2D Camera2D;

namespace whal {

struct PointLight;
struct BoxLight;
struct Radiance;
struct Transform2D;
struct ShadowLight;

void drawLights(Camera2D worldCamera);

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
    void update(Camera2D worldCamera);

private:
    int mPositionUniform;
};

class ShadowLightSystem : public ecs::ISystem<Transform2D, ShadowLight> {
public:
    void update();
};

}  // namespace whal

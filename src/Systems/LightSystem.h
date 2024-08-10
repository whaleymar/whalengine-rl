#pragma once

#include "Events/Events.h"
#include "Sys/System.h"
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

class PointLightSystem : public ecs::ISystem<Transform2D, PointLight>, public IListen<ShaderReloadEvent, true> {
public:
    void update();
    void onEvent(ShaderReloadEvent) override;

private:
    int mPositionUniform;
};

class BoxLightSystem : public ecs::ISystem<Transform2D, BoxLight>, public IListen<ShaderReloadEvent, true> {
public:
    void update();
    void onEvent(ShaderReloadEvent) override;

private:
    int mPositionUniform;
    int mHalflenUniform;
    int mRadiusUniform;
};

class RadianceLightSystem : public ecs::ISystem<Transform2D, Radiance>, public IListen<ShaderReloadEvent, true> {
public:
    void update(Camera2D worldCamera);
    void onEvent(ShaderReloadEvent) override;

private:
    int mPositionUniform;
};

class ShadowLightSystem : public ecs::ISystem<Transform2D, ShadowLight>, public IListen<ShaderReloadEvent, true> {
public:
    void update();
    void onEvent(ShaderReloadEvent) override;

private:
    int mLightPosUniform;
    int mRadiusUniform;
};

}  // namespace whal

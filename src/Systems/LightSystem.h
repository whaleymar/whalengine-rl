#pragma once

#include "Events/Events.h"
#include "Sys/IListen.h"
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

class PointLightSystem : public ecs::ISystem<Transform2D, PointLight>, public IListen<evt::ShaderReload, true> {
public:
    void drawEntities();
    void onEvent(evt::ShaderReload) override;

private:
    int mPositionUniform;
    // int mLightDepthUniform;
    // int mOcclusionDepthUniform;
};

class BoxLightSystem : public ecs::ISystem<Transform2D, BoxLight>, public IListen<evt::ShaderReload, true> {
public:
    void drawEntities();
    void onEvent(evt::ShaderReload) override;

private:
    int mPositionUniform;
    int mHalflenUniform;
    int mRadiusUniform;
    int mLightDepthUniform;
    int mOcclusionDepthUniform;
};

class RadianceLightSystem : public ecs::ISystem<Transform2D, Radiance>, public IListen<evt::ShaderReload, true> {
public:
    void drawEntities(Camera2D worldCamera);
    void onEvent(evt::ShaderReload) override;

private:
    int mPositionUniform;
};

class ShadowLightSystem : public ecs::ISystem<Transform2D, ShadowLight>, public IListen<evt::ShaderReload, true> {
public:
    void drawEntities();
    void onEvent(evt::ShaderReload) override;

private:
    int mLightPosUniform;
    int mRadiusUniform;
    int mLightDepthUniform;
    int mOcclusionDepthUniform;
    int mAllDepthUniform;
};

}  // namespace whal

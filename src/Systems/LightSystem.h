#pragma once

#include "Events/Events.h"
#include "Sys/IListen.h"
#include "whalECS/src/ECS.h"

typedef struct Shader Shader;
typedef struct Camera2D Camera2D;

namespace whal {

struct PointLight;
struct BoxLight;
struct Transform;
struct ShadowLight;
struct Invisible;

class PointLightSystem : public ecs::ISystem<Transform, PointLight, ecs::Exclude<Invisible>>,
                         public ecs::IRenderLight,
                         public IListen<evt::ShaderReload, true> {
public:
    void draw(const gfx::RenderContext&) const override;
    void onEvent(evt::ShaderReload) override;

private:
    int mPositionUniform;
    // int mLightDepthUniform;
    // int mOcclusionDepthUniform;
};

class BoxLightSystem : public ecs::ISystem<Transform, BoxLight, ecs::Exclude<Invisible>>,
                       public ecs::IRenderLight,
                       public IListen<evt::ShaderReload, true> {
public:
    void draw(const gfx::RenderContext&) const override;
    void onEvent(evt::ShaderReload) override;

private:
    int mPositionUniform;
    int mHalflenUniform;
    int mRadiusUniform;
    int mLightDepthUniform;
    int mOcclusionDepthUniform;
};

class ShadowLightSystem : public ecs::ISystem<Transform, ShadowLight, ecs::Exclude<Invisible>>,
                          public ecs::IRenderLight,
                          public IListen<evt::ShaderReload, true> {
public:
    void draw(const gfx::RenderContext&) const override;
    void onEvent(evt::ShaderReload) override;

private:
    int mLightPosUniform;
    int mRadiusUniform;
    int mLightDepthUniform;
    int mDepthBufUniform;
    int mOcclDepthBufUniform;
};

}  // namespace whal

#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

struct PointLight;
struct BoxLight;
struct Transform;
struct ShadowLight;
struct Invisible;

class PointLightSystem : public ecs::ISystem<Transform, PointLight, ecs::Exclude<Invisible>>, public ecs::IRenderLight {
public:
    void draw(const gfx::RenderContext&) const override;
};

class BoxLightSystem : public ecs::ISystem<Transform, BoxLight, ecs::Exclude<Invisible>>, public ecs::IRenderLight {
public:
    void draw(const gfx::RenderContext&) const override;
};

class ShadowLightSystem : public ecs::ISystem<Transform, ShadowLight, ecs::Exclude<Invisible>>, public ecs::IRenderLight {
public:
    void draw(const gfx::RenderContext&) const override;
};

}  // namespace whal

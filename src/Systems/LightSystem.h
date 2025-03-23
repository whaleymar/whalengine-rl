#pragma once

#include "Components/Light.h"
#include "Components/Tags.h"
#include "Components/Transform.h"
#include "whalECS/src/ECS.h"

namespace whal {

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

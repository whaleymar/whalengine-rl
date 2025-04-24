#pragma once

#include "Components/IDrawable.h"
#include "Components/Tags.h"
#include "Components/Transform.h"
#include "Gfx/IRender.h"
#include "whalECS/src/ECS.h"

namespace whal {

class SpriteRenderSystem : public ecs::ISystem<Transform, ecs::MatchTrait<IDrawable>, ecs::Exclude<Invisible>>,
                           public IRender,
                           public ecs::AttrExcludeChildren {
public:
    SpriteRenderSystem();
    void draw(const gfx::EntityRenderInfo& entity, const gfx::RenderContext& ctx) const override;
    void addToQueue(gfx::RenderQueue&) const override;
};

}  // namespace whal

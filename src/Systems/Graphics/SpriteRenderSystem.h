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
    void draw(const gfx::EntityRenderInfo& entity, const gfx::RenderContext& ctx) const override;
    void addToQueue(gfx::RenderQueue&) const override;
};

// draw functions for Drawable engine components:
void drawSprite(ecs::Entity entity, const gfx::RenderContext& ctx);
void queueSprite(ecs::Entity entity, gfx::RenderQueue& queue);

void drawRectangle(ecs::Entity entity, const gfx::RenderContext& ctx);
void queueRectangle(ecs::Entity entity, gfx::RenderQueue& queue);

}  // namespace whal

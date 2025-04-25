#pragma once

#include "Components/IDrawable.h"
#include "Components/Transform.h"
#include "Gfx/IRender.h"
#include "whalECS/src/ECS.h"

namespace whal {

// Exclude<Invisible> doesn't mix well with AttrExcludeChildren, because if a parent is invisible, drawable children will just get added to the system
// instead. So instead I check for the invisible tag manually when queueing entities.
class SpriteRenderSystem : public ecs::ISystem<Transform, ecs::MatchTrait<IDrawable>>, public IRender, public ecs::AttrExcludeChildren {
public:
    SpriteRenderSystem();
    void draw(const gfx::EntityRenderInfo& entity, const gfx::RenderContext& ctx) const override;
    void addToQueue(gfx::RenderQueue&) const override;
};

}  // namespace whal

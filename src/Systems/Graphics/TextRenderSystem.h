#pragma once

#include "Components/Draw.h"
#include "Components/Tags.h"
#include "Components/Transform.h"
#include "whalECS/src/ECS.h"

namespace rl {
typedef struct Font Font;
}

namespace whal {

class TextRenderSystem : public ecs::ISystem<TextSprite, Transform, ecs::Exclude<Invisible>>, public ecs::IRender {
public:
    TextRenderSystem();
    ~TextRenderSystem();
    void draw(const gfx::EntityRenderInfo& entity, const gfx::RenderContext& ctx) const override;
    void addToQueue(gfx::RenderQueue&) const override;

private:
    rl::Font* mFont;
};

}  // namespace whal

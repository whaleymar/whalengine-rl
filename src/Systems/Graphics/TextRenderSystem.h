#pragma once

#include "whalECS/src/ECS.h"

namespace rl {
typedef struct Font Font;
}

namespace whal {

struct DrawText;
struct Transform;
struct Invisible;

class TextRenderSystem : public ecs::ISystem<DrawText, Transform, ecs::Exclude<Invisible>>, public ecs::IRender {
public:
    TextRenderSystem();
    ~TextRenderSystem();
    void draw(const gfx::EntityRenderInfo& entity, const gfx::RenderContext& ctx) const override;
    void addToQueue(gfx::RenderQueue&) const override;

private:
    rl::Font* mFont;
};

}  // namespace whal

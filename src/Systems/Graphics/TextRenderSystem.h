#pragma once

#include "whalECS/src/ECS.h"

namespace rl {
typedef struct Font Font;
}

namespace whal {

struct DrawText;
struct Transform;

class TextRenderSystem : public ecs::ISystem<DrawText, Transform>, public ecs::IRender {
public:
    TextRenderSystem();
    ~TextRenderSystem();
    void draw(const gfx::EntityRenderInfo& entity, const gfx::RenderContext& ctx) const override;
    void addToQueue(std::vector<gfx::EntityRenderInfo>&) const override;

private:
    rl::Font* mFont;
};

}  // namespace whal

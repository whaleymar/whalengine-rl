#pragma once

#include "whalECS/src/ECS.h"

typedef struct Font Font;

namespace whal {

struct DrawText;
struct Transform2D;

class TextRenderSystem : public ecs::ISystem<DrawText, Transform2D>, public ecs::IRender {
public:
    TextRenderSystem();
    ~TextRenderSystem();
    void draw(ecs::Entity entity, const gfx::RenderContext ctx) const override;
    void addToQueue(std::vector<gfx::EntityRenderInfo>&) const override;

private:
    Font* mFont;
};

}  // namespace whal

#include "Components/Draw.h"

#include <cstring>
#include <raylib.h>
#include "Map/Tiled.h"
#include "json.hpp"

#include "Components/Transform.h"

#include "Gfx/Frame.h"
#include "Gfx/Texture.h"

#include "Util/Print.h"
#include "Util/Vector.h"

namespace whal {

Sprite::Sprite(Frame frame, Color color_) : frameSize(frame.size), atlasPosition(frame.atlasPosition), color(color_) {}

Expected<Sprite> Sprite::fromPath(const char* spritePath, Color color_) {
    const auto& spriteTexture = TextureManager::getAtlas(TEXNAME_SPRITE);
    auto frame = spriteTexture.getFrame(spritePath);
    if (frame) {
        return Sprite(*frame, color_);
    }
    return Error(whal_format("Couldn't find {} in texture atlas", spritePath));
}

void Sprite::setFrame(Frame frame) {
    frameSize = frame.size;
    atlasPosition = frame.atlasPosition;
}

void Sprite::loadImpl(ecs::Entity entity, void* data) {
    const LoadContext& ctx = *static_cast<LoadContext*>(data);
    Sprite sprite = entity.has<Sprite>() ? entity.get<Sprite>() : Sprite{};

    s32 rotationDegrees;
    if (tryReadInt(ctx.values, "rotationDegrees", &rotationDegrees)) {
        entity.get<Transform>().rotationDegrees = rotationDegrees;
    }

    tryReadColor(ctx.values, "Color", &sprite.color);

    f32 brightness;
    if (tryReadFloat(ctx.values, "Brightness", &brightness)) {
        sprite.color.scale(brightness);
    }

    std::string spritePath = "";
    if (ctx.values.contains("Sprite")) {
        spritePath = ctx.values["Sprite"];
        std::replace(spritePath.begin(), spritePath.end(), '\\', '/');
    }
    auto eSprite = Sprite::fromPath(spritePath.c_str());
    if (eSprite.isExpected()) {
        sprite.frameSize = eSprite.value().frameSize;
        sprite.atlasPosition = eSprite.value().atlasPosition;
        entity.add(sprite);
    } else {
        print("Error: Coudn't find frame for sprite:", spritePath);
    }
}

DrawRect::DrawRect(Color color_, Vector2i frameSize_) : frameSize(frameSize_), color(color_) {}

void DrawRect::loadImpl(ecs::Entity entity, void* data) {
    const LoadContext& ctx = *static_cast<LoadContext*>(data);
    DrawRect draw = entity.has<DrawRect>() ? entity.get<DrawRect>() : DrawRect{};
    draw.frameSize = ctx.entityData.size;

    tryReadColor(ctx.values, "Color", &draw.color);

    f32 brightness;
    if (tryReadFloat(ctx.values, "Brightness", &brightness)) {
        draw.color.scale(brightness);
    }
    entity.add(draw);
}

void DrawText::loadImpl(ecs::Entity entity, void* data) {
    DrawText text = entity.has<DrawText>() ? entity.get<DrawText>() : DrawText{};
    const LoadContext& ctx = *static_cast<LoadContext*>(data);

    tryReadColor(ctx.values, "color", &text.color);
    tryReadString(ctx.values, "text", &text.text);
    tryReadBool(ctx.values, "center", &text.isCentered);
    text.frameSize = ctx.entityData.size;
    entity.add(text);
}

}  // namespace whal

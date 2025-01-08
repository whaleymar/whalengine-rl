#include "Components/Draw.h"

#include <cstring>
#include <raylib.h>
#include <rfl/json.hpp>
#include "Map/Tiled.h"
#include "Map/TiledParse.h"

#include "Components/Transform.h"

#include "Gfx/Frame.h"
#include "Gfx/Texture.h"

#include "Util/Print.h"
#include "Util/Vector.h"

namespace whal {

Expected<Sprite> Sprite::fromPath(const char* spritePath, Color color_) {
    const auto& spriteTexture = TextureManager::getAtlas(TEXNAME_SPRITE);
    auto frame = spriteTexture.getFrame(spritePath);
    if (frame) {
        return fromFrame(*frame, color_);
    }
    return Error(whal_format("Couldn't find {} in texture atlas", spritePath));
}

Sprite Sprite::fromFrame(Frame frame, Color color) {
    return Sprite{
        .frameSize = frame.size.as<f32>(),
        .atlasPosition = frame.atlasPosition.as<f32>(),
        .color = color,
    };
}

void Sprite::setFrame(Frame frame) {
    frameSize = frame.size.as<f32>();
    atlasPosition = frame.atlasPosition.as<f32>();
}

Frame Sprite::getFrame() const {
    return Frame(atlasPosition.as<s32>(), frameSize.as<s32>());
}

void Sprite::setMask(Frame frame) {
    maskPosRelative = frame.atlasPosition.as<f32>() - atlasPosition;
}

void Sprite::setMask(const char* maskAtlasPath) {
    const auto& spriteTexture = TextureManager::getAtlas(TEXNAME_SPRITE);
    auto frame = spriteTexture.getFrame(maskAtlasPath);
    if (frame) {
        setMask(*frame);
    } else {
        print("couldn't find in atlas:", maskAtlasPath);
    }
}

void Sprite::removeMask() {
    maskPosRelative = Vector2f::ZERO;
}

void Sprite::setFlag(flag f) {
    flags |= f;
}

void Sprite::resetFlag(flag f) {
    flags = (flags & ~f);
}

bool Sprite::isFlagSet(flag f) const {
    return (flags & f) > 0;
}

namespace stl {
template <class ForwardIt, class T = typename std::iterator_traits<ForwardIt>::value_type>
void replace(ForwardIt first, ForwardIt last, const T& old_value, const T& new_value) {
    for (; first != last; ++first)
        if (*first == old_value)
            *first = new_value;
}
}  // namespace stl

void Sprite::loadImpl(ecs::Entity entity, const LoadContext& ctx) {
    Sprite sprite = entity.has<Sprite>() ? entity.get<Sprite>() : Sprite{};

    tryRead(*ctx.values, "Color", &sprite.color);

    f32 brightness;
    if (tryRead(*ctx.values, "Brightness", &brightness)) {
        sprite.color.scale(brightness);
    }

    std::string spritePath = "";
    if (tryRead(*ctx.values, "Sprite", &spritePath)) {
        stl::replace(spritePath.begin(), spritePath.end(), '\\', '/');
    }
    auto eSprite = Sprite::fromPath(spritePath.c_str());
    if (eSprite.isExpected()) {
        sprite.frameSize = eSprite.value().frameSize;
        sprite.atlasPosition = eSprite.value().atlasPosition;
        entity.add(sprite);
    } else {
        print("Error: Coudn't find frame for sprite:", spritePath);
    }

    // how to rfl::json :
    // print(rfl::json::write(sprite));
    // print(rfl::json::write(entity.get<Transform>()));
}

DrawRect DrawRect::create(Color color, Vector2i frameSize) {
    return DrawRect{
        .frameSize = frameSize,
        .color = color,
    };
}

void DrawRect::loadImpl(ecs::Entity entity, const LoadContext& ctx) {
    DrawRect draw = entity.has<DrawRect>() ? entity.get<DrawRect>() : DrawRect{};
    draw.frameSize = ctx.entityData.size;

    tryRead(*ctx.values, "Color", &draw.color);

    f32 brightness;
    if (tryRead(*ctx.values, "Brightness", &brightness)) {
        draw.color.scale(brightness);
    }
    entity.add(draw);
}

void DrawText::loadImpl(ecs::Entity entity, const LoadContext& ctx) {
    DrawText text = entity.has<DrawText>() ? entity.get<DrawText>() : DrawText{};

    tryRead(*ctx.values, "color", &text.color);
    tryRead(*ctx.values, "text", &text.text);
    tryRead(*ctx.values, "center", &text.isCentered);
    text.frameSize = ctx.entityData.size;
    entity.add(text);
}

}  // namespace whal

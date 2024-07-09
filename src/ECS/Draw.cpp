#include "ECS/Draw.h"

#include <raylib.h>
#include <sstream>

#include "ECS/Lifetime.h"
#include "ECS/Light.h"
#include "ECS/Transform.h"
#include "Gfx/Texture.h"
#include "Settings.h"
#include "Util/Vector.h"
#include "whalECS/src/ECS.h"

#include "Systems/System.h"

namespace whal {

Color hexStringARGBToColor(std::string hexString) {
    s32 r, g, b;
    hexString = hexString.erase(0, 3);  // remove "#" and alpha
    std::istringstream(hexString.substr(0, 2)) >> std::hex >> r;
    std::istringstream(hexString.substr(2, 2)) >> std::hex >> g;
    std::istringstream(hexString.substr(4, 2)) >> std::hex >> b;

    return Color(r, g, b, 255);
}

constexpr f32 getPixelSize(const s32 frameSize, const f32 scale) {
    return static_cast<f32>(frameSize) * PIXELS_PER_TEXEL * scale;
}

IDraw::IDraw(Depth depth_, Color color_, Vector2i frameSizeTexels, Shaders shader_)
    : color(color_), depth(depth_), shader(shader_), mFrameSizeTexels(frameSizeTexels){};

void IDraw::setAlpha(u8 alpha) {
    color.a = alpha;
}

void IDraw::setFrameSize(s32 frameSizeX, s32 frameSizeY) {
    mFrameSizeTexels = {frameSizeX, frameSizeY};
}

void IDraw::setFrameSize(Vector2i frameSize) {
    mFrameSizeTexels = frameSize;
}

void IDraw::setColor(Color rgb) {
    color = rgb;
}

Sprite::Sprite(Depth depth_, Frame frame, Color color_, Shaders shader_)
    : IDraw(depth_, color_, frame.dimensionsTexels, shader_), atlasPositionTexels(frame.atlasPositionTexels) {}

void Sprite::setFrame(Frame frame) {
    setFrameSize(frame.dimensionsTexels);
    atlasPositionTexels = frame.atlasPositionTexels;
}

Draw::Draw(Color color_, Vector2i frameSizeTexels_, Depth depth_, Shaders shader_) : IDraw(depth_, color_, frameSizeTexels_, shader_) {}

DrawText::DrawText(const char* string, Depth depth_, Color color_, Vector2i frameSizeTexels, Shaders shader_)
    : IDraw(depth_, color_, frameSizeTexels, shader_), text(string) {}

f32 FadeOut::getIntensity() const {
    f32 t = secondsRemaining / time;
    if (secondsRemaining <= 0) {
        t = 0;
    }
    return t;
}

u8 FadeOut::getAlpha() const {
    f32 t = getIntensity();
    return static_cast<u8>(clamp(255.0f * myLerp(startAlpha, endAlpha, 1 - t), 0.0f, 255.0f));
}

Expected<ecs::Entity> makeSilhouetteFromSprite(ecs::Entity entity, f32 lifetime, Corrade::Containers::Optional<Color> color) {
    auto eEntity = System::world->entity();
    if (!eEntity.isExpected()) {
        return eEntity.error();
    }

    auto sil = eEntity.value();
    auto _ = ecs::DeferActivate(sil);

    sil.add(entity.get<Transform2D>());
    auto sprite = entity.get<Sprite>();
    if (color) {
        sprite.color = *color;
    }
    sprite.shader = Shaders::Silhouette;
    sprite.depth = Depth::BehindPlayer;
    sil.add(sprite);

    sil.add(FadeOut(lifetime));
    sil.add(Lifetime(lifetime));
    sil.add(PointLight{std::max(sprite.getFrameSizeTexels().x(), sprite.getFrameSizeTexels().y()), sprite.getFrameSizeTexels().y() / 2});

    return sil;
}

Expected<ecs::Entity> makeSilhouetteFromDraw(ecs::Entity entity, f32 lifetime, Corrade::Containers::Optional<Color> color) {
    auto eEntity = System::world->entity();
    if (!eEntity.isExpected()) {
        return eEntity.error();
    }

    auto sil = eEntity.value();
    auto _ = ecs::DeferActivate(sil);

    sil.add(entity.get<Transform2D>());
    auto draw = entity.get<Draw>();
    if (color) {
        draw.color = *color;
    }
    draw.shader = Shaders::Silhouette;
    draw.depth = Depth::BehindPlayer;
    sil.add(draw);

    sil.add(FadeOut(lifetime));
    sil.add(Lifetime(lifetime));
    sil.add(PointLight{std::max(draw.getFrameSizeTexels().x(), draw.getFrameSizeTexels().y()), draw.getFrameSizeTexels().y() / 2});
    sil.add(Radiance({std::max(draw.getFrameSizeTexels().x(), draw.getFrameSizeTexels().y()), draw.getFrameSizeTexels().y() / 2, draw.color}));

    return sil;
}

}  // namespace whal

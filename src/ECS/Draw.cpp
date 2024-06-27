#include "ECS/Draw.h"

#include <raylib.h>
#include <sstream>

#include "ECS/Lifetime.h"
#include "ECS/Light.h"
#include "ECS/Tags.h"
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

IDraw::IDraw(Depth depth_, Color color_, Vector2i frameSizeTexels) : depth(depth_), color(color_), mFrameSizeTexels(frameSizeTexels){};

void IDraw::setAlpha(u8 alpha) {
    color.a = alpha;
}

Sprite::Sprite(Depth depth_, Frame frame, Color color_)
    : IDraw(depth_, color_, frame.dimensionsTexels), atlasPositionTexels(frame.atlasPositionTexels) {}

void Sprite::setFrameSize(s32 frameSizeX, s32 frameSizeY) {
    mFrameSizeTexels = {frameSizeX, frameSizeY};
}

void Sprite::setFrameSize(Vector2i frameSize) {
    mFrameSizeTexels = frameSize;
}

void Sprite::setFrame(Frame frame) {
    setFrameSize(frame.dimensionsTexels);
    atlasPositionTexels = frame.atlasPositionTexels;
}

void Sprite::setColor(Color color_) {
    color = color_;
}

Draw::Draw(Color color_, Vector2i frameSizeTexels_, Depth depth_) : IDraw(depth_, color_, frameSizeTexels_) {}

void Draw::setFrameSize(s32 frameSizeX, s32 frameSizeY) {
    mFrameSizeTexels = {frameSizeX, frameSizeY};
}

void Draw::setFrameSize(Vector2i frameSize) {
    mFrameSizeTexels = frameSize;
}

void Draw::setColor(Color rgb) {
    color = rgb;
}

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
    sil.add(sprite);

    sil.add<Silhouette>();
    sil.add(FadeOut(lifetime));
    sil.add(Lifetime(lifetime));
    sil.add(PointLight{std::max(sprite.getFrameSizeTexels().x(), sprite.getFrameSizeTexels().y()) * PIXELS_PER_TEXEL,
                       sprite.getFrameSizeTexels().y() / 2 * PIXELS_PER_TEXEL});

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
    sil.add(draw);

    sil.add<Silhouette>();
    sil.add(FadeOut(lifetime));
    sil.add(Lifetime(lifetime));
    sil.add(PointLight{std::max(draw.getFrameSizeTexels().x(), draw.getFrameSizeTexels().y()) * PIXELS_PER_TEXEL,
                       draw.getFrameSizeTexels().y() / 2 * PIXELS_PER_TEXEL});
    sil.add(Radiance({std::max(draw.getFrameSizeTexels().x(), draw.getFrameSizeTexels().y()) * PIXELS_PER_TEXEL,
                      draw.getFrameSizeTexels().y() / 2 * PIXELS_PER_TEXEL, draw.color}));

    return sil;
}

}  // namespace whal

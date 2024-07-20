#include "Components/Draw.h"

#include <cstring>
#include <raylib.h>
#include <sstream>

#include "Components/Lifetime.h"
#include "Components/Light.h"
#include "Components/Transform.h"
#include "Gfx/Texture.h"
#include "Settings.h"
#include "Util/Vector.h"
#include "whalECS/src/ECS.h"

#include "Sys/System.h"

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
    : color(color_), depth(depth_), shader(shader_), mFrameSizeTexels(frameSizeTexels) {};

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

DrawRect::DrawRect(Color color_, Vector2i frameSizeTexels_, Depth depth_, Shaders shader_) : IDraw(depth_, color_, frameSizeTexels_, shader_) {}

DrawText::DrawText(const char* string, Color color_, Vector2i frameSizeTexels_, bool centered)
    : text(string), color(color_), frameSizeTexels(frameSizeTexels_), isCentered(centered) {}

// rect by default
Draw::Draw(Depth depth, Vector2i frameSizeTexels, Shaders shader, Color color) : mRect(color, frameSizeTexels, depth, shader), mTag(DrawTag::Rect) {}

Draw::Draw(DrawRect rect) : mRect(rect), mTag(DrawTag::Rect) {}

Draw::Draw(Sprite sprite) : mSprite(sprite), mTag(DrawTag::Sprite) {}

Draw::Draw(DrawBezierQuad bezier) : mBezierQuad(bezier), mTag(DrawTag::BezierQuad) {}

Draw::Draw(DrawStraightLine line) : mLine(line), mTag(DrawTag::Line) {}

Draw::Draw(const Draw& other) {
    std::memcpy(this, &other, sizeof(other));
}

Draw& Draw::operator=(const Draw& other) {
    if (this == &other) {
        return *this;
    }
    std::memcpy(this, &other, sizeof(other));
    return *this;
}

DrawRect& Draw::getRect() {
    assert(mTag == DrawTag::Rect && "trying to run Draw::getRect on something else");
    return mRect;
}

Sprite& Draw::getSprite() {
    assert(mTag == DrawTag::Sprite && "trying to run Draw::getSprite on something else");
    return mSprite;
}

DrawBezierQuad& Draw::getBezierQuad() {
    assert(mTag == DrawTag::BezierQuad && "trying to run Draw::getBezierQuad on something else");
    return mBezierQuad;
}

DrawStraightLine& Draw::getLine() {
    assert(mTag == DrawTag::Line && "trying to run Draw::getLine on something else");
    return mLine;
}

Vector2i Draw::getFrameSizeTexels() const {
    switch (mTag) {
    case DrawTag::Rect:
        return mRect.getFrameSizeTexels();
    case DrawTag::Sprite:
        return mSprite.getFrameSizeTexels();
    case DrawTag::BezierQuad:
        return Vector2i();  // unused so idc that it's inaccurate
    case DrawTag::Line:
        return Vector2i();  // unused so idc that it's inaccurate
    }
}

Depth Draw::getDepth() const {
    switch (mTag) {
    case DrawTag::Rect:
        return mRect.depth;
    case DrawTag::Sprite:
        return mSprite.depth;
    case DrawTag::BezierQuad:
        return mBezierQuad.depth;
    case DrawTag::Line:
        return mLine.depth;
    }
}

Shaders Draw::getShader() const {
    switch (mTag) {
    case DrawTag::Rect:
        return mRect.shader;
    case DrawTag::Sprite:
        return mSprite.shader;
    case DrawTag::BezierQuad:
        return mBezierQuad.shader;
    case DrawTag::Line:
        return mLine.shader;
    }
}

void Draw::setAlpha(u8 alpha) {
    switch (mTag) {
    case DrawTag::Rect:
        mRect.setAlpha(alpha);
        break;
    case DrawTag::Sprite:
        mSprite.setAlpha(alpha);
        break;
    case DrawTag::BezierQuad:
        mBezierQuad.color.a = alpha;
        break;
    case DrawTag::Line:
        mLine.color.a = alpha;
        break;
    }
}

void Draw::setFrameSize(s32 x, s32 y) {
    switch (mTag) {
    case DrawTag::Rect:
        mRect.setFrameSize(x, y);
        break;
    case DrawTag::Sprite:
        mSprite.setFrameSize(x, y);
        break;
    case DrawTag::BezierQuad:
        break;
    case DrawTag::Line:
        break;
    }
}

void Draw::setFrameSize(Vector2i frameSize) {
    switch (mTag) {
    case DrawTag::Rect:
        mRect.setFrameSize(frameSize);
        break;
    case DrawTag::Sprite:
        mSprite.setFrameSize(frameSize);
        break;
    case DrawTag::BezierQuad:
        break;
    case DrawTag::Line:
        break;
    }
}

void Draw::setColor(Color color) {
    switch (mTag) {
    case DrawTag::Rect:
        mRect.setColor(color);
        break;
    case DrawTag::Sprite:
        mSprite.setColor(color);
        break;
    case DrawTag::BezierQuad:
        mBezierQuad.color = color;
        break;
    case DrawTag::Line:
        mLine.color = color;
        break;
    }
}

void Draw::setScale(Vector2f scale) {
    switch (mTag) {
    case DrawTag::Rect:
        mRect.scale = scale;
        break;
    case DrawTag::Sprite:
        mSprite.scale = scale;
        break;
    case DrawTag::BezierQuad:
        break;
    case DrawTag::Line:
        break;
    }
}

Vector2f Draw::getScale() const {
    switch (mTag) {
    case DrawTag::Rect:
        return mRect.scale;
    case DrawTag::Sprite:
        return mSprite.scale;
    case DrawTag::BezierQuad:
    case DrawTag::Line:
        return Vector2f(1.0, 1.0);
    }
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
    auto sprite = entity.get<Draw>().getSprite();
    if (color) {
        sprite.color = *color;
    }
    sprite.shader = Shaders::Silhouette;
    sprite.depth = Depth::BehindPlayer;
    sil.add(Draw(sprite));

    sil.add(FadeOut(lifetime));
    sil.add(Lifetime(lifetime));
    sil.add(PointLight{std::max(sprite.getFrameSizeTexels().x, sprite.getFrameSizeTexels().y), sprite.getFrameSizeTexels().y / 2});

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
    auto draw = entity.get<Draw>().getRect();
    if (color) {
        draw.color = *color;
    }
    draw.shader = Shaders::Silhouette;
    draw.depth = Depth::BehindPlayer;
    sil.add(Draw(draw));

    sil.add(FadeOut(lifetime));
    sil.add(Lifetime(lifetime));
    sil.add(PointLight{std::max(draw.getFrameSizeTexels().x, draw.getFrameSizeTexels().y), draw.getFrameSizeTexels().y / 2});
    sil.add(Radiance({std::max(draw.getFrameSizeTexels().x, draw.getFrameSizeTexels().y), draw.getFrameSizeTexels().y / 2, draw.color}));

    return sil;
}

}  // namespace whal

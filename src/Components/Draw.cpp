#include "Components/Draw.h"

#include <cstring>
#include <raylib.h>
#include <sstream>

#include "Components/Lifetime.h"
#include "Components/Light.h"
#include "Components/Transform.h"
#include "Gfx/Texture.h"
#include "Util/Print.h"
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

IDraw::IDraw(Depth depth_, Color color_, Vector2i frameSize, Shaders shader_)
    : color(color_), depth(depth_), shader(shader_), mFrameSize(frameSize) {};

void IDraw::setAlpha(u8 alpha) {
    color.a = alpha;
}

void IDraw::setFrameSize(s32 frameSizeX, s32 frameSizeY) {
    mFrameSize = {frameSizeX, frameSizeY};
}

void IDraw::setFrameSize(Vector2i frameSize) {
    mFrameSize = frameSize;
}

void IDraw::setColor(Color rgb) {
    color = rgb;
}

Sprite::Sprite(Depth depth_, Frame frame, Color color_, Shaders shader_)
    : IDraw(depth_, color_, frame.size, shader_), atlasPosition(frame.atlasPosition) {}

Expected<Sprite> Sprite::fromPath(const char* spritePath, Depth depth_, Color color_, Shaders shader_) {
    const auto& spriteTexture = TextureManager::getAtlas(TEXNAME_SPRITE);
    auto frame = spriteTexture.getFrame(spritePath);
    if (frame) {
        return Sprite(depth_, *frame, color_, shader_);
    }
    return Error(whal_format("Couldn't find {} in texture atlas", spritePath));
}

void Sprite::setFrame(Frame frame) {
    setFrameSize(frame.size);
    atlasPosition = frame.atlasPosition;
}

DrawRect::DrawRect(Color color_, Vector2i frameSize_, Depth depth_, Shaders shader_) : IDraw(depth_, color_, frameSize_, shader_) {}

DrawText::DrawText(const char* string, Color color_, Vector2i frameSize_, bool centered)
    : text(string), color(color_), frameSize(frameSize_), isCentered(centered) {}

// rect by default
Draw::Draw(Depth depth, Vector2i frameSize, Shaders shader, Color color) : mRect(color, frameSize, depth, shader), mTag(DrawTag::Rect) {}

Draw::Draw(DrawRect rect, u32 flags) : mRect(rect), mTag(DrawTag::Rect), mPostProcessFlags(flags) {}

Draw::Draw(Sprite sprite, u32 flags) : mSprite(sprite), mTag(DrawTag::Sprite), mPostProcessFlags(flags) {}

Draw::Draw(DrawBezierQuad bezier, u32 flags) : mBezierQuad(bezier), mTag(DrawTag::BezierQuad), mPostProcessFlags(flags) {}

Draw::Draw(DrawStraightLine line, u32 flags) : mLine(line), mTag(DrawTag::Line), mPostProcessFlags(flags) {}

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

Vector2i Draw::getFrameSize() const {
    switch (mTag) {
    case DrawTag::Rect:
        return mRect.getFrameSize();
    case DrawTag::Sprite:
        return mSprite.getFrameSize();
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
        mLine.scale = scale;
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
        return mLine.scale;
    }
}

}  // namespace whal

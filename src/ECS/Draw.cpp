#include "ECS/Draw.h"

#include <sstream>

#include "Gfx/Texture.h"
#include "Settings.h"
#include "Util/Vector.h"

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

}  // namespace whal

#include "Components/Draw.h"

#include <cstring>
#include <raylib.h>
#include <sstream>

#include "Gfx/Texture.h"
#include "Util/Print.h"
#include "Util/Vector.h"
#include "whalECS/src/ECS.h"

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

}  // namespace whal

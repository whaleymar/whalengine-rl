#include "Components/Draw.h"

#include <cstring>
#include <raylib.h>

#include "Gfx/Frame.h"
#include "Gfx/Texture.h"

#include "Util/Print.h"
#include "Util/Vector.h"

namespace whal {

IDraw::IDraw(Color color_) : color(color_) {}

Sprite::Sprite(Frame frame, Color color_) : IDraw(color_), frameSize(frame.size), atlasPosition(frame.atlasPosition) {}

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

DrawRect::DrawRect(Color color_, Vector2i frameSize_) : IDraw(color_), frameSize(frameSize_) {}

}  // namespace whal

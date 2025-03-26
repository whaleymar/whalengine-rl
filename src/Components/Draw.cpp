#include "Components/Draw.h"

#include <cstring>
#include <raylib.h>

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

DrawRect DrawRect::create(Color color, Vector2i frameSize) {
    return DrawRect{
        .frameSize = frameSize,
        .color = color,
    };
}

}  // namespace whal

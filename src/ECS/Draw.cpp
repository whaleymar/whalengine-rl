#include "ECS/Components/Draw.h"

#include "Gfx/Texture.h"
#include "Settings.h"
#include "Util/Vector.h"

namespace whal {

constexpr f32 getPixelSize(const s32 frameSize, const f32 scale) {
    return static_cast<f32>(frameSize) * PIXELS_PER_TEXEL * scale;
}

IDraw::IDraw(Depth depth_, RGB rgb, Vector2i frameSizeTexels) : depth(depth_), color(rgb), mFrameSizeTexels(frameSizeTexels){};

Sprite::Sprite(Depth depth_, Frame frame, RGB rgb) : IDraw(depth_, rgb, frame.dimensionsTexels), atlasPositionTexels(frame.atlasPositionTexels) {}

void Sprite::setFrameSize(s32 frameSizeX, s32 frameSizeY) {
    mFrameSizeTexels = {frameSizeX, frameSizeY};
    isVertsUpdateNeeded = true;
}

void Sprite::setFrameSize(Vector2i frameSize) {
    mFrameSizeTexels = frameSize;
    isVertsUpdateNeeded = true;
}

void Sprite::setFrame(Frame frame) {
    setFrameSize(frame.dimensionsTexels);
    atlasPositionTexels = frame.atlasPositionTexels;
}

void Sprite::setColor(RGB rgb) {
    color = rgb;
    isVertsUpdateNeeded = true;
}

void Sprite::updateVertices(bool isFlipX) {
    const f32 width = getPixelSize(mFrameSizeTexels.x(), scale.x());
    const f32 height = getPixelSize(mFrameSizeTexels.y(), scale.y());

    Vector2f sheetSize = GLResourceManager::getInstance().getTexture(TEXNAME_SPRITE).getSize();
    f32 scaleX = 1 / sheetSize.x();
    f32 scaleY = 1 / sheetSize.y();

    Vector2f texelOffset = toFloatVec(atlasPositionTexels);
    f32 xMin = texelOffset.x() * scaleX;
    f32 xMax = (texelOffset.x() + mFrameSizeTexels.x()) * scaleX;
    f32 yMin = texelOffset.y() * scaleY;
    f32 yMax = (texelOffset.y() + mFrameSizeTexels.y()) * scaleY;

    if (isFlipX) {
        mVertices = MakeRectVerticesRGBUV(width, height, depth, color, xMax, xMin, yMax, yMin);
    } else {
        mVertices = MakeRectVerticesRGBUV(width, height, depth, color, xMin, xMax, yMax, yMin);
    }
    isVertsUpdateNeeded = false;
}

Draw::Draw(RGB color_, Vector2i frameSizeTexels_, Depth depth_) : IDraw(depth_, color_, frameSizeTexels_) {
    updateVertices();
}

void Draw::setFrameSize(s32 frameSizeX, s32 frameSizeY) {
    mFrameSizeTexels = {frameSizeX, frameSizeY};
    updateVertices();
}

void Draw::setFrameSize(Vector2i frameSize) {
    mFrameSizeTexels = frameSize;
    updateVertices();
}

void Draw::updateVertices() {
    const f32 width = getPixelSize(mFrameSizeTexels.x(), 1.0);
    const f32 height = getPixelSize(mFrameSizeTexels.y(), 1.0);

    mVertices = MakeRectVerticesRGB(width, height, depth, color);
}

void Draw::setColor(RGB rgb) {
    color = rgb;
    updateVertices();
}

}  // namespace whal

#pragma once

#include <raylib.h>

#include "Gfx/Depth.h"
#include "Gfx/Texture.h"
#include "Util/Vector.h"

namespace whal {

class Texture;

Color hexStringARGBToColor(std::string hexstring);

namespace Colors {

inline static Color Clear = {0, 0, 0, 0};
inline static Color Magenta = {255, 0, 255, 255};
inline static Color Emerald = {80, 204, 96, 255};
inline static Color Purple = {198, 51, 242, 255};
inline static Color Pink = {242, 116, 217, 255};

}  // namespace Colors

// hard coded as rectangles until I need something else
struct IDraw {
    IDraw(Depth depth_, Color color_, Vector2i frameSizeTexels);
    Depth depth;
    Color color;
    Vector2f scale = {1, 1};

    Vector2i getFrameSizeTexels() const { return mFrameSizeTexels; }

protected:
    Vector2i mFrameSizeTexels;
};

struct Sprite : public IDraw {
    Sprite(Depth depth_ = Depth::Player, Frame frame = {}, Color color_ = WHITE);

    Vector2i atlasPositionTexels;
    // bool isVertsUpdateNeeded = true;  // anim, size, color, and/or scale changed

    void setFrame(Frame frame);
    void setFrameSize(s32 x, s32 y);
    void setFrameSize(Vector2i frameSize);
    void setColor(Color color_);
};

struct Draw : public IDraw {
    Draw(Color color_ = WHITE, Vector2i frameSizeTexels_ = {8, 8}, Depth depth_ = Depth::Player);

    void setFrameSize(s32 x, s32 y);
    void setFrameSize(Vector2i frameSize);
    void setColor(Color color_);
};

struct DrawDebug : public Draw {};

}  // namespace whal

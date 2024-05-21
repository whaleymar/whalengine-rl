#pragma once

#include "Gfx/Texture.h"
#include "Util/Vector.h"

namespace whal {

class Texture;

// hard coded as rectangles until I need something else
struct IDraw {
    IDraw(Depth depth_, RGB rgb, Vector2i frameSizeTexels);
    Depth depth;
    RGB color;
    u32 nVertices = N_VERTS_RECT;
    Vector2f scale = {1, 1};
    Vao vao;
    Vbo vbo;

    Vector2i getFrameSizeTexels() const { return mFrameSizeTexels; }

protected:
    Vector2i mFrameSizeTexels;
};

struct Sprite : public IDraw {
    Sprite(Depth depth_ = Depth::Player, Frame frame = {}, RGB rgb = Color::WHITE);

    Vector2i atlasPositionTexels;
    bool isVertsUpdateNeeded = true;  // anim, size, color, and/or scale changed

    VertArrayRectRGBUV getVertices() const { return mVertices; }
    void updateVertices(bool flipX = false);
    void setFrame(Frame frame);
    void setFrameSize(s32 x, s32 y);
    void setFrameSize(Vector2i frameSize);
    void setColor(RGB rgb);

private:
    VertArrayRectRGBUV mVertices;
};

struct Draw : public IDraw {
    Draw(RGB color_ = Color::WHITE, Vector2i frameSizeTexels_ = {8, 8}, Depth depth_ = Depth::Player);

    VertArrayRectRGB getVertices() const { return mVertices; }
    void updateVertices();
    void setFrameSize(s32 x, s32 y);
    void setFrameSize(Vector2i frameSize);
    void setColor(RGB rgb);

private:
    VertArrayRectRGB mVertices;
};

}  // namespace whal

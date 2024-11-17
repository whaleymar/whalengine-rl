#pragma once

#include <raylib.h>
#include <string>

#include "Gfx/Depth.h"
#include "Util/Vector.h"

template <typename T>
class Expected;

namespace whal {

struct Frame;

enum class DrawTag { Rect, Sprite, BezierQuad, Line };

struct IDraw {
    IDraw() = default;
    IDraw(Color color_);

    Color color = WHITE;
    f32 brightness = 1.0f;
};

struct Sprite : public IDraw {
    Sprite() = default;
    Sprite(Frame frame, Color color_ = WHITE);

    static Expected<Sprite> fromPath(const char* spritePath, Color color_ = WHITE);
    void setFrame(Frame frame);

    Vector2i frameSize;
    Vector2i atlasPosition;
};

struct DrawRect : public IDraw {
    DrawRect(Color color_ = WHITE, Vector2i frameSize_ = {8, 8});

    Vector2i frameSize;
};

struct DrawBezierQuad {
    Vector2i controlPointOffset;
    Vector2i endPointOffset;
    Color color = WHITE;
    f32 thickness = 1.0;
    f32 brightness = 1.0f;
    Depth depth = Depth::Level;
};

struct DrawStraightLine {
    s32 length;
    Color color = WHITE;
    f32 thickness = 1.0;
    f32 brightness = 1.0f;
    bool isRotateAboutCenter = false;
};

struct DrawText {
    std::string text;
    Vector2i frameSize;
    Color color = WHITE;
    f32 brightness = 1.0f;
    bool isCentered = false;
};

}  // namespace whal

#pragma once

#include <raylib.h>

#include "Gfx/Depth.h"
#include "Gfx/ShaderManager.h"
#include "Gfx/Texture.h"
#include "Settings.h"
#include "Util/Vector.h"
#include "whalECS/src/Expected.h"

inline Color operator+(const Color& left, const Color& right) {
    return Color(left.r + right.r, left.g + right.g, left.b + right.b, left.a + right.a);
}

inline Color operator*=(Color& left, const f32 f) {
    left.r = left.r * f;
    left.g = left.g * f;
    left.b = left.b * f;
    left.a = left.a * f;
    return left;
}

namespace whal {

Color hexStringARGBToColor(std::string hexstring);

namespace ecs {
class Entity;
}

namespace Colors {

inline static Color Clear = {0, 0, 0, 0};
inline static Color Magenta = {255, 0, 255, 255};
inline static Color Emerald = {80, 204, 96, 255};
inline static Color Purple = {198, 51, 242, 255};
// inline static Color Pink = {242, 116, 217, 255};
inline static Color Pink = {255, 170, 255, 255};
inline static Color LightBlue = {85, 255, 255, 255};

inline Color lerp(Color first, Color second, f32 t) {
    return Color{static_cast<u8>(myLerp(static_cast<f32>(first.r), static_cast<f32>(second.r), t)),
                 static_cast<u8>(myLerp(static_cast<f32>(first.g), static_cast<f32>(second.g), t)),
                 static_cast<u8>(myLerp(static_cast<f32>(first.b), static_cast<f32>(second.b), t)),
                 static_cast<u8>(myLerp(static_cast<f32>(first.a), static_cast<f32>(second.a), t))};
}

}  // namespace Colors

// hard coded as rectangles until I need something else
struct IDraw {
    IDraw(Depth depth_, Color color_, Vector2i frameSize, Shaders shader_);
    Color color;
    Vector2f scale = {1, 1};
    Depth depth;
    Shaders shader;

    Vector2i getFrameSize() const { return mFrameSize; }
    void setAlpha(u8 alpha);
    void setFrameSize(s32 x, s32 y);
    void setFrameSize(Vector2i frameSize);
    void setColor(Color color_);

protected:
    Vector2i mFrameSize;
};

struct Sprite : public IDraw {
    Sprite(Depth depth_ = Depth::Player, Frame frame = {}, Color color_ = WHITE, Shaders shader_ = Shaders::Default);
    static Expected<Sprite> fromPath(const char* spritePath, Depth depth_ = Depth::Player, Color color_ = WHITE, Shaders shader_ = Shaders::Default);
    void setFrame(Frame frame);

    Vector2i atlasPosition;
    bool isRotateAboutCenter = false;
};

struct DrawRect : public IDraw {
    DrawRect(Color color_ = WHITE, Vector2i frameSize_ = {8, 8}, Depth depth_ = Depth::Player, Shaders shader_ = Shaders::Default);
};

struct DrawBezierQuad {
    Vector2i controlPointOffset;
    Vector2i endPointOffset;
    Color color = WHITE;
    f32 thickness = 1.0;
    Depth depth = Depth::Level;
    Shaders shader = Shaders::Default;
};

struct DrawStraightLine {
    s32 length;
    Color color = WHITE;
    f32 thickness = 1.0;
    Vector2f scale = Vector2f::one;
    Depth depth = Depth::Level;
    bool isRotateAboutCenter = false;
    Shaders shader = Shaders::Default;
};

// NOTE: modifying Depth or Target Texture requires the component to be removed and re-added
class Draw {
public:
    // TODO text can go here once I have a font that matches the target resolution
    // probably won't get rid of other version, because I can use that for dialogue system?
    enum class DrawTag { Rect, Sprite, BezierQuad, Line };  // text too?

    Draw(Depth depth = Depth::Player, Vector2i frameSize = {PIXELS_PER_TILE, PIXELS_PER_TILE}, Shaders shader = Shaders::Default,
         Color color = WHITE);
    Draw(DrawRect rect, u32 flags = 0);
    Draw(Sprite sprite, u32 flags = 0);
    Draw(DrawBezierQuad bezier, u32 flags = 0);
    Draw(DrawStraightLine line, u32 flags = 0);

    Draw(const Draw& other);
    Draw& operator=(const Draw& other);

    DrawTag getTag() const { return mTag; }
    DrawRect& getRect();
    Sprite& getSprite();
    DrawBezierQuad& getBezierQuad();
    DrawStraightLine& getLine();

    Vector2i getFrameSize() const;
    Depth getDepth() const;
    Shaders getShader() const;
    void setPostProcessFlags(u32 flags) { mPostProcessFlags = flags; }
    void setAlpha(u8 alpha);
    void setFrameSize(s32 x, s32 y);
    void setFrameSize(Vector2i frameSize);
    void setColor(Color color_);
    void setScale(Vector2f scale);
    Vector2f getScale() const;

private:
    union {
        DrawRect mRect;
        Sprite mSprite;
        DrawBezierQuad mBezierQuad;
        DrawStraightLine mLine;
    };
    DrawTag mTag;
    u32 mPostProcessFlags = 0;
};

struct DrawText {
    DrawText(const char* string = "", Color color_ = WHITE, Vector2i frameSize = {8, 8}, bool centered = false);

    std::string text;
    Color color;
    Vector2f scale = {1, 1};
    Vector2i frameSize;
    bool isCentered;
    Depth depth = Depth::Debug;
};

}  // namespace whal

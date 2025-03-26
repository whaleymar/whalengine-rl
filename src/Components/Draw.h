#pragma once

#include <string>

#include "Gfx/Color.h"
#include "Gfx/Depth.h"
#include "Gfx/Frame.h"
#include "Util/Vector.h"

template <typename T>
class Expected;

namespace whal {

struct Frame;

enum class DrawTag { Rect, Sprite, BezierQuad, Line };

struct Sprite {
    // controls settings in the main sprite shader
    enum flag : u32 {
        None = 0,
        Silhouette = 1 << 0,
        MaskBlendAdditive = 1 << 1,
        // mask blending (subtract)
        // outline
    };

    Vector2f frameSize;
    Vector2f atlasPosition;
    Color color = Colors::White;

    // RESEARCH i should have some "mask self" flag so I can trivially use the intersection of the mask position with the Sprite's frame to do
    // rectangle masks would be nice for clipping/tweening
    Vector2f maskPosRelative = Vector2f::ZERO;  // relative position of the sprite mask in the texture atlas (zero for no mask)
    u32 flags = flag::None;

    static Expected<Sprite> fromPath(const char* spritePath, Color color_ = Colors::White);
    static Sprite fromFrame(Frame frame, Color color_ = Colors::White);

    void setFrame(Frame frame);
    Frame getFrame() const;
    void setMask(Frame frame);
    void setMask(const char* maskAtlasPath);
    void removeMask();

    void setFlag(flag f);
    void resetFlag(flag f);
    bool isFlagSet(flag f) const;
};

struct DrawRect {
    static DrawRect create(Color color = Colors::White, Vector2i frameSize = {8, 8});

    Vector2i frameSize;
    Color color = Colors::White;
};

struct DrawBezierQuad {
    Vector2i controlPointOffset;
    Vector2i endPointOffset;
    Color color = Colors::White;
    f32 thickness = 1.0;
    Depth depth = Depth::Level;
};

struct DrawStraightLine {
    s32 length;
    Color color = Colors::White;
    f32 thickness = 1.0;

    // dotted line params:
    s32 segmentLength = 0;       // 0 == draw as single line segment
    s32 segmentGapLength = 0;    // 0 == draw as single line segment
    f32 segmentCycleTime = 0.0;  // For animating a dotted line's movement. In Seconds.
    bool isRotateAboutCenter = false;
};

struct DrawText {
    std::string text;
    Vector2i frameSize;
    Color color = Colors::White;
    bool isCentered = false;
    bool isWrapped = true;
};

struct SpriteOutline {
    Color color = Colors::White;
};

}  // namespace whal

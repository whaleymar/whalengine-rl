#pragma once

#include <string>

#include "Gfx/Color.h"
#include "Gfx/Depth.h"
#include "Map/ComponentFactory.h"
#include "Util/Vector.h"

template <typename T>
class Expected;

namespace whal {

struct Frame;

enum class DrawTag { Rect, Sprite, BezierQuad, Line };

struct Sprite : ISerialize<Sprite, ComponentFactory> {
    // TODO get rid of constructor, use static fromFrame
    Sprite() = default;
    Sprite(Frame frame, Color color_ = Colors::White);

    static Expected<Sprite> fromPath(const char* spritePath, Color color_ = Colors::White);
    void setFrame(Frame frame);

    Vector2i frameSize;
    Vector2i atlasPosition;
    Color color = Colors::White;

    static void loadImpl(ecs::Entity entity, void* data);
};

struct DrawRect : ISerialize<DrawRect, ComponentFactory> {
    DrawRect(Color color_ = Colors::White, Vector2i frameSize_ = {8, 8});

    Vector2i frameSize;
    Color color = Colors::White;

    static void loadImpl(ecs::Entity entity, void* data);
};

struct DrawBezierQuad {
    Vector2i controlPointOffset;
    Vector2i endPointOffset;
    Color color = Colors::White;
    f32 thickness = 1.0;
    f32 brightness = 1.0f;
    Depth depth = Depth::Level;
};

struct DrawStraightLine {
    s32 length;
    Color color = Colors::White;
    f32 thickness = 1.0;
    f32 brightness = 1.0f;
    bool isRotateAboutCenter = false;
};

struct DrawText : ISerialize<DrawText, ComponentFactory> {
    std::string text;
    Vector2i frameSize;
    Color color = Colors::White;
    f32 brightness = 1.0f;
    bool isCentered = false;

    static void loadImpl(ecs::Entity entity, void* data);
};

}  // namespace whal

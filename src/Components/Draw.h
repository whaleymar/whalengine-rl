#pragma once

#include <string>

#include "Gfx/Color.h"
#include "Gfx/Depth.h"
#include "Gfx/Frame.h"
#include "Map/ComponentFactory.h"
#include "Util/Vector.h"

template <typename T>
class Expected;

namespace whal {

struct Frame;

enum class DrawTag { Rect, Sprite, BezierQuad, Line };

struct Sprite : ISerialize<Sprite, ComponentFactory> {
    // controls settings in the main sprite shader
    enum flag : u32 {
        None = 0,
        Silhouette = 1 << 0,
        MaskBlendAdditive = 1 << 1,
        // mask blending (subtract)
        // outline
    };

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

    Vector2f frameSize;
    Vector2f atlasPosition;
    Color color = Colors::White;

    Vector2f maskPosRelative = Vector2f::ZERO;  // relative position of the sprite mask in the texture atlas (zero for no mask)
    u32 flags = flag::None;

    static void loadImpl(ecs::Entity entity, const LoadContext& ctx);
};
// constexpr bool asdf = is_base_of_template<ISerialize, Sprite>::value;

struct DrawRect : ISerialize<DrawRect, ComponentFactory> {
    static DrawRect create(Color color = Colors::White, Vector2i frameSize = {8, 8});

    Vector2i frameSize;
    Color color = Colors::White;

    static void loadImpl(ecs::Entity entity, const LoadContext& ctx);
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
    bool isRotateAboutCenter = false;
};

struct DrawText : ISerialize<DrawText, ComponentFactory> {
    std::string text;
    Vector2i frameSize;
    Color color = Colors::White;
    bool isCentered = false;

    static void loadImpl(ecs::Entity entity, const LoadContext& ctx);
};

struct SpriteOutline : ISerialize<SpriteOutline, ComponentFactory> {
    Color color = Colors::White;
};
REGISTER_SERIALIZE(SpriteOutline);

}  // namespace whal

#pragma once

#include "Components/Draw.h"
#include "Components/Transform.h"
#include "Events/Events.h"
#include "Sys/System.h"
#include "whalECS/src/ECS.h"

typedef struct Texture Texture;
typedef Texture Texture2D;
typedef struct Shader Shader;
typedef struct Color Color;
typedef struct Camera2D Camera2D;

namespace whal {

struct ColorLerp;
struct ScaleLerp;
struct FadeOut;
struct DrawText;
enum class TextureID;
struct Invisible;

class GfxSystem : public ecs::ISystem<Transform2D, Draw, ecs::Exclude<Invisible>>, public IListen<ShaderReloadEvent, true> {
public:
    // hopefully will become its own component soon
    struct PreciseTransform {
        Vector2f position;
        Vector2i rounded;
        f32 rotationDegrees = 0.0;      // counterclockwise
        Facing facing = Facing::Right;  // draw calls flipped if facing left

        static PreciseTransform fromTrans(Transform2D trans) {
            return PreciseTransform{
                .position = trans.position.as<f32>(), .rounded = trans.position, .rotationDegrees = trans.rotationDegrees, .facing = trans.facing};
        }

        static PreciseTransform fromBoth(Transform2D trans, PrecisePosition pos) {
            return PreciseTransform{
                .position = pos.position, .rounded = trans.position, .rotationDegrees = trans.rotationDegrees, .facing = trans.facing};
        }
    };

    struct DrawInfo {
        PreciseTransform trans;
        Draw draw;
        ecs::EntityID id;
    };

    void onEvent(ShaderReloadEvent) override;
    void drawEntities(Camera2D worldCamera);

private:
    void sortEntities(Vector2i cameraPos);
    std::vector<DrawInfo> mSortedEntities;
};

class DrawTextSystem : public ecs::ISystem<Transform2D, DrawText> {
public:
    DrawTextSystem();
    void drawEntities(Color tint);
};

class DrawDebugSystem : public ecs::ISystem<Transform2D, DrawDebug> {
public:
    void drawEntities();
};

class FadeOutSystem : public ecs::ISystem<Transform2D, Draw, FadeOut>, public ecs::IUpdate {
public:
    void update() override;
};

class ColorLerpSystem : public ecs::ISystem<Draw, ColorLerp>, public ecs::IUpdate {
public:
    void update() override;
};

class ScaleLerpSystem : public ecs::ISystem<Draw, ScaleLerp>, public ecs::IUpdate {
public:
    void update() override;
};

}  // namespace whal

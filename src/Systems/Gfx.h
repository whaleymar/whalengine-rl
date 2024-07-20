#pragma once

#include <forward_list>

#include "Util/Vector.h"
#include "whalECS/src/ECS.h"

typedef struct Texture Texture;
typedef Texture Texture2D;
typedef struct Shader Shader;
typedef struct Color Color;

namespace whal {

struct Transform2D;
struct DrawDebug;
class Draw;
struct ColorLerp;
struct ScaleLerp;
struct FadeOut;
struct DrawText;

class GfxSystem : public ecs::ISystem<Transform2D, Draw>, public ecs::IMonitorSystem {
    struct DrawInfo {
        ecs::Entity entity;
        f32 depth;
        s16 shaderIx;
    };

public:
    void onAdd(const ecs::Entity) override;
    void onRemove(const ecs::Entity) override;

    static bool isBelow(const DrawInfo& first, const DrawInfo& second);
    void drawEntities();
    void drawEntity(ecs::Entity entity, const Texture2D& spriteTexture, const Vector2f cameraPosF);

private:
    std::forward_list<DrawInfo> mSorted;
    std::vector<DrawInfo> mAddedEntities;
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

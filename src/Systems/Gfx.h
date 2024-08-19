#pragma once

#include <forward_list>

#include "Gfx/Depth.h"
#include "Gfx/ShaderManager.h"
#include "Util/Vector.h"
#include "whalECS/src/ECS.h"

typedef struct Texture Texture;
typedef Texture Texture2D;
typedef struct Shader Shader;
typedef struct Color Color;
typedef struct Camera2D Camera2D;

namespace whal {

struct Transform2D;
struct DrawDebug;
class Draw;
struct ColorLerp;
struct ScaleLerp;
struct FadeOut;
struct DrawText;
enum class TextureID;
struct Invisible;

class GfxSystem : public ecs::ISystem<Transform2D, Draw, ecs::Exclude<Invisible>>, public ecs::IMonitorSystem {
    struct DrawInfo {
        ecs::Entity entity;
        f32 depth;
        Depth depthId;
        s16 shaderIx;
    };

    // separate entity lists for each target texture
    struct Layer {
        Layer(Shaders shader_) : shader(shader_) {}

        std::forward_list<DrawInfo> sorted;
        std::vector<DrawInfo> toSort;
        std::forward_list<DrawInfo>::iterator iter;

        Shaders shader;

        void update();
    };

public:
    void onAdd(const ecs::Entity) override;
    void onRemove(const ecs::Entity) override;

    void drawEntities(Camera2D worldCamera);

private:
    static bool isBelow(const DrawInfo& first, const DrawInfo& second);
    std::forward_list<GfxSystem::DrawInfo>::iterator drawEntities(Layer& layer, std::forward_list<DrawInfo>::iterator startIt);
    void drawEntity(ecs::Entity entity, const Texture2D& spriteTexture, const Vector2f cameraPosF);
    Layer& getLayer(TextureID texId);

    Layer mLayerNormal = Layer(Shaders::Default);
    Layer mLayerBloom = Layer(Shaders::Bloom);
    Layer mLayerGlow = Layer(Shaders::Glow);
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

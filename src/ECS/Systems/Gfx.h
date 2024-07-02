#pragma once

#include <forward_list>

#include "Util/Vector.h"
#include "whalECS/src/ECS.h"

typedef struct Texture Texture;
typedef Texture Texture2D;
typedef struct Shader Shader;

namespace whal {

struct Transform2D;
struct Sprite;
struct Draw;
struct DrawDebug;
struct FadeOut;

class SpriteSystem : public ecs::ISystem<Transform2D, Sprite>, public ecs::IMonitorSystem {
public:
    void onAdd(const ecs::Entity) override;
    void onRemove(const ecs::Entity) override;

    void drawEntities();
    void drawEntity(ecs::Entity entity, const Texture2D& spriteTexture, const Vector2f cameraPosF);

private:
    std::forward_list<ecs::Entity> mSorted;
};

class DrawSystem : public ecs::ISystem<Transform2D, Draw> {
public:
    void drawEntities();
};

class DrawDebugSystem : public ecs::ISystem<Transform2D, DrawDebug> {
public:
    void drawEntities();
};

class FadeOutSystem : public ecs::ISystem<Transform2D, FadeOut>, public ecs::IUpdate {
public:
    void update() override;
};

}  // namespace whal

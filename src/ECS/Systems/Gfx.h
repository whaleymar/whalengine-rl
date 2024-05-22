#pragma once

#include <forward_list>

#include "whalECS/src/ECS.h"

namespace whal {

struct Transform2D;
struct Sprite;
struct Draw;
struct DrawDebug;

class SpriteSystem : public ecs::ISystem<Transform2D, Sprite> {
public:
    void onAdd(const ecs::Entity) override;
    void onRemove(const ecs::Entity) override;
    void drawEntities();

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

}  // namespace whal

#pragma once

#include <forward_list>

#include "whalECS/src/ECS.h"

namespace whal {

struct Transform;
struct Sprite;
struct Draw;

class SpriteSystem : public ecs::ISystem<Transform, Sprite> {
public:
    void onAdd(const ecs::Entity) override;
    void onRemove(const ecs::Entity) override;
    void drawEntities();

private:
    std::forward_list<ecs::Entity> mSorted;
};

class DrawSystem : public ecs::ISystem<Transform, Draw> {
public:
    void drawEntities();
};

}  // namespace whal

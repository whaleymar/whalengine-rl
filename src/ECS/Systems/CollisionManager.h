#pragma once

#include "Physics/QuadTree/Quadtree.h"
#include "whalECS/src/ECS.h"

namespace whal {

constexpr s32 WORLD_HALFLEN_PIXELS = 10000;

class Collider;

#ifndef NDEBUG
void drawColliders();
#endif

struct Transform2D;

class QuadTreeSystem : public ecs::ISystem<Collider>, public ecs::IMonitorSystem {
public:
    static void updatePosition(ecs::Entity entity, AABB* colliderShape, Vector2i nextPosition);
    static void updatePosition(ecs::Entity entity, AABB* colliderShape, Transform2D nextPosition);
    static std::vector<ecs::Entity> query(const AABB& aabb);

    void onAdd(ecs::Entity entity) override;
    void onRemove(ecs::Entity entity) override;

private:
    static inline qtree::QuadTree mQuadTree = qtree::QuadTree(AABB(Vector2i(0, 0), Vector2i(WORLD_HALFLEN_PIXELS, WORLD_HALFLEN_PIXELS)));
};

}  // namespace whal

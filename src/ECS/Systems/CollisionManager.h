#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

class Collider;
class AABB;

#ifndef NDEBUG
void drawColliders();
#endif

struct Transform2D;

class QuadTreeSystem : public ecs::ISystem<Collider>, public ecs::IMonitorSystem {
public:
    static void updatePosition(ecs::Entity entity, AABB* colliderShape, Vector2i nextPosition);
    static void updatePosition(ecs::Entity entity, AABB* colliderShape, Transform2D nextPosition);
    static std::vector<ecs::Entity> query(const AABB& aabb);

    // Rebuild QuadTree with new size. All entities in the system are added to the new tree.
    // This is SLOW and should only run during loads.
    static void rebuild(s32 width, s32 height);

    void onAdd(ecs::Entity entity) override;
    void onRemove(ecs::Entity entity) override;
};

}  // namespace whal

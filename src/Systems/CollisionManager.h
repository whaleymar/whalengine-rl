#pragma once

#include "Physics/CollisionLayer.h"
#include "Util/Types.h"
#include "whalECS/src/ECS.h"

namespace whal {

class Collider;
class AABB;
struct RaycastHit;

#ifndef NDEBUG
void drawColliders();
#endif

struct Transform2D;

class QuadTreeSystem : public ecs::ISystem<Collider>, public ecs::IMonitorSystem {
public:
    QuadTreeSystem();
    static void updatePosition(ecs::Entity entity, AABB& colliderShape, Transform2D nextPosition);
    static void updateShape(ecs::Entity entity, const AABB& previousShape, const AABB& newShape);
    static std::vector<ecs::Entity> query(const AABB& aabb);

    // Note: does not detect entities whose collider contains the ray's origin point
    // Note: direction does not have to be normalized
    static RaycastHit raycast(Vector2f origin, Vector2f direction, f32 maxDistance, u16 layerMask = CollisionLayer::ALL);
    static RaycastHit circlecast(Vector2f origin, Vector2f direction, f32 maxDistance, f32 radius, u16 layerMask = CollisionLayer::ALL);

    // Rebuild QuadTree with new size. All entities in the system are added to the new tree.
    // This is SLOW and should only run during loads.
    static void rebuild(s32 width, s32 height);

    void onAdd(ecs::Entity entity) override;
    void onRemove(ecs::Entity entity) override;
};

}  // namespace whal

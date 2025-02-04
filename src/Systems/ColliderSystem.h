#pragma once

#include "Physics/CollisionLayer.h"
#include "Util/Types.h"
#include "Util/Vector.h"
#include "whalECS/src/ECS.h"

namespace whal {

class Collider;
class AABB;
struct RaycastHit;

#ifndef NDEBUG
void drawColliders();
#endif

struct Transform;
class PhysicsSystem;

class ColliderSystem : public ecs::ISystem<Collider, Transform>, public ecs::IMonitorSystem {
public:
    ColliderSystem();
    static void updatePosition(ecs::Entity entity, AABB& colliderShape, Transform nextPosition, Vector2i colliderOffset);
    static void updateShape(ecs::Entity entity, const AABB& previousShape, const AABB& newShape);
    static std::vector<ecs::Entity> query(const AABB& aabb);

    // Note: does not detect entities whose collider contains the ray's origin point
    // Note: direction does not have to be normalized
    static RaycastHit raycast(Vector2f origin, Vector2f direction, f32 maxDistance, u16 layerMask = CollisionLayer::ALL);
    static RaycastHit circlecast(Vector2f origin, Vector2f direction, f32 maxDistance, f32 radius, u16 layerMask = CollisionLayer::ALL);

    static void setIsIgnoreCollision(ecs::Entity first, ecs::Entity second, bool ignore = true);
    static bool isIgnoreCollision(ecs::Entity first, ecs::Entity second);

    // Rebuild QuadTree with new size. All entities in the system are added to the new tree.
    // This is SLOW and should only run during loads.
    static void rebuild(s32 width, s32 height);

    void onAdd(ecs::Entity entity) override;
    void onRemove(ecs::Entity entity) override;

    friend PhysicsSystem;

private:
    // Syncs colliders with their entity's transform (in case it was altered by another system).
    // Called immediately before the Physics system updates.
    // If a Transform's isManuallyMoved flag is set, then the collider teleports to the transform.
    // Otherwise, the collider tries to move to the transform within the physics system. If it is
    // stopped by another physics object, the transform is updated to its new position.

    // TODO this needs to take the physics system's entity list? It used to operate on entities with Velocity
    // and it worked fine? Now that velocity isn't required it's WAY slower.
    static void syncColliders();
};

}  // namespace whal

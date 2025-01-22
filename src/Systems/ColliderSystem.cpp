#include "ColliderSystem.h"
#include <raylib.h>

#include "Components/Collider.h"
#include "Components/Transform.h"
#include "Components/TriggerZone.h"

#include "Physics/CollisionLayer.h"
#include "Physics/HitInfo.h"
#include "Physics/QuadTree/Quadtree.h"

#include "Systems/TriggerSystem.h"

#include "Util/CameraUtil.h"
#include "Util/Vector.h"

#ifndef NDEBUG
#include "Gfx/Color.h"
#endif

namespace whal {

constexpr s32 WORLD_HALFLEN_PIXELS_DEFAULT = 10000;

static qtree::QuadTree makeDefaultQuadtree() {
    return qtree::QuadTree(AABB(Vector2i(0, 0), Vector2i(WORLD_HALFLEN_PIXELS_DEFAULT, WORLD_HALFLEN_PIXELS_DEFAULT)));
}

static qtree::QuadTree QUAD_TREE = makeDefaultQuadtree();

ColliderSystem::ColliderSystem() {
    QUAD_TREE = makeDefaultQuadtree();
}

#ifndef NDEBUG
void drawColliders() {
    for (const auto [entityid, entity] : ColliderSystem::getEntities()) {
        const auto collider = entity.get<Collider>();
        Color color;
        if (collider.isFeatherBody()) {
            color = Colors::Magenta;
        } else if (collider.isHeavyBody()) {
            color = Colors::Red;
        } else if (collider.isRigidBody()) {
            color = Colors::Pink;
        } else {
            color = Colors::Blue;
        }
        collider.getShape().draw(color);
    }

    for (const auto& [entityid, entity] : TriggerSystem::getEntities()) {
        entity.get<Trigger>().shape.draw(Colors::Emerald);
    }
}
#endif

void ColliderSystem::updatePosition(ecs::Entity entity, AABB& colliderShape, Transform nextPosition, Vector2i colliderOffset) {
    QUAD_TREE.remove(entity);
    colliderShape.setPosition(nextPosition, colliderOffset);
    QUAD_TREE.add(entity);
}

void ColliderSystem::updateShape(ecs::Entity entity, const AABB& previousShape, const AABB& newShape) {
    QUAD_TREE.remove(entity, previousShape);
    QUAD_TREE.add(entity, newShape);
}

std::vector<ecs::Entity> ColliderSystem::query(const AABB& aabb) {
    return QUAD_TREE.query(aabb);
}

RaycastHit ColliderSystem::raycast(Vector2f origin, Vector2f direction, f32 maxDistance, u16 layerMask) {
    return QUAD_TREE.raycast(origin, direction, maxDistance, layerMask);
}

RaycastHit ColliderSystem::circlecast(Vector2f origin, Vector2f direction, f32 maxDistance, f32 radius, u16 layerMask) {
    return QUAD_TREE.circlecast(origin, direction, maxDistance, radius, layerMask);
}

void ColliderSystem::rebuild(s32 width, s32 height) {
    QUAD_TREE = qtree::QuadTree(AABB(Vector2i(0, 0), Vector2i(width / 2, height / 2)));
    for (auto [entityid, entity] : getEntities()) {
        QUAD_TREE.add(entity);
    }
}

void ColliderSystem::onAdd(ecs::Entity entity) {
    // RESEARCH unhandled edge case: fails if we try to create an entity beyond quadtree bounds.
    auto& collider = entity.get<Collider>();
    collider.setEntity(entity);
    if (collider.isFeatherBody() || collider.isRigidBody()) {
        entity.add<Momentum>();
    }
    QUAD_TREE.add(entity);
}

void ColliderSystem::onRemove(ecs::Entity entity) {
    QUAD_TREE.remove(entity);
}

}  // namespace whal

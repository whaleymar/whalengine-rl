#include "CollisionManager.h"
#include <raylib.h>

#include "Components/Transform.h"
#include "Components/TriggerZone.h"

#include "Physics/CollisionLayer.h"
#include "Physics/QuadTree/Quadtree.h"

#include "Systems/TagTrackers.h"
#include "Systems/TriggerSystem.h"

#include "Util/Color.h"
#include "Util/Vector.h"

namespace whal {

constexpr s32 WORLD_HALFLEN_PIXELS_DEFAULT = 10000;

static qtree::QuadTree QUAD_TREE = qtree::QuadTree(AABB(Vector2i(0, 0), Vector2i(WORLD_HALFLEN_PIXELS_DEFAULT, WORLD_HALFLEN_PIXELS_DEFAULT)));

#ifndef NDEBUG
void drawColliders() {
    auto cameraPos = getCameraPositionPrecise();
    // auto cameraPos = toFloatVec(getCameraPosition());
    for (const auto [entityid, entity] : QuadTreeSystem::getEntitiesMutable()) {
        const auto collider = entity.get<Collider>();
        Color color;
        if (collider.isActor()) {
            color = Colors::Magenta;
        } else if (collider.isSolid()) {
            color = RED;
        } else if (collider.isSemiSolid()) {
            color = Colors::Pink;
        } else {
            color = BLUE;
        }
        collider.getShape().draw(cameraPos, color);
    }

    for (const auto& [entityid, entity] : TriggerSystem::getEntitiesMutable()) {
        entity.get<Trigger>().shape.draw(cameraPos, Colors::Emerald);
    }
}
#endif

void QuadTreeSystem::updatePosition(ecs::Entity entity, AABB& colliderShape, Transform2D nextPosition) {
    QUAD_TREE.remove(entity);
    colliderShape.setPosition(nextPosition);
    QUAD_TREE.add(entity);
}

void QuadTreeSystem::updateShape(ecs::Entity entity, const AABB& previousShape, const AABB& newShape) {
    QUAD_TREE.remove(entity, previousShape);
    QUAD_TREE.add(entity, newShape);
}

std::vector<ecs::Entity> QuadTreeSystem::query(const AABB& aabb) {
    return QUAD_TREE.query(aabb);
}

void QuadTreeSystem::rebuild(s32 width, s32 height) {
    QUAD_TREE = qtree::QuadTree(AABB(Vector2i(0, 0), Vector2i(width / 2, height / 2)));
    for (auto [entityid, entity] : getEntitiesMutable()) {
        QUAD_TREE.add(entity);
    }
}

void QuadTreeSystem::onAdd(ecs::Entity entity) {
    // RESEARCH unhandled edge case: fails if we try to create an entity beyond quadtree bounds.
    auto& collider = entity.get<Collider>();
    collider.setEntity(entity);
    if (collider.isActor() || collider.isSemiSolid()) {
        entity.add<Momentum>();
    }
    QUAD_TREE.add(entity);
}

void QuadTreeSystem::onRemove(ecs::Entity entity) {
    QUAD_TREE.remove(entity);
}

}  // namespace whal

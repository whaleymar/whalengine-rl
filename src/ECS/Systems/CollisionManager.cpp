#include "CollisionManager.h"
#include <raylib.h>

#include "ECS/Draw.h"
#include "ECS/Systems/TagTrackers.h"
#include "ECS/Systems/TriggerSystem.h"
#include "ECS/Transform.h"
#include "ECS/TriggerZone.h"
#include "Physics/CollisionLayer.h"
#include "Util/Vector.h"

namespace whal {

#ifndef NDEBUG
void drawColliders() {
    auto cameraPos = getCameraPositionPrecise();
    // auto cameraPos = toFloatVec(getCameraPosition());
    for (const auto [entityid, entity] : QuadTreeSystem::getEntitiesRef()) {
        auto collider = &entity.get<Collider>();
        Color color;
        if (collider->isActor()) {
            color = Colors::Magenta;
        } else if (collider->isSolid()) {
            color = RED;
        } else if (collider->isSemiSolid()) {
            color = Colors::Pink;
        } else {
            color = BLUE;
        }
        collider->getShape().draw(cameraPos, color);
    }

    for (const auto& [entityid, entity] : TriggerSystem::getEntitiesRef()) {
        entity.get<Trigger>().shape.draw(cameraPos, Colors::Emerald);
    }
}
#endif

void QuadTreeSystem::updatePosition(ecs::Entity entity, AABB* colliderShape, Vector2i nextPosition) {
    mQuadTree.remove(entity);
    colliderShape->setPosition(nextPosition);
    mQuadTree.add(entity);
}

void QuadTreeSystem::updatePosition(ecs::Entity entity, AABB* colliderShape, Transform2D nextPosition) {
    // TODO if we're at the world border, don't move
    mQuadTree.remove(entity);
    colliderShape->setPosition(nextPosition);
    mQuadTree.add(entity);
}

std::vector<ecs::Entity> QuadTreeSystem::query(const AABB& aabb) {
    return mQuadTree.query(aabb);
}

void QuadTreeSystem::onAdd(ecs::Entity entity) {
    entity.get<Collider>().setEntity(entity);
    mQuadTree.add(entity);
}

void QuadTreeSystem::onRemove(ecs::Entity entity) {
    mQuadTree.remove(entity);
}

}  // namespace whal

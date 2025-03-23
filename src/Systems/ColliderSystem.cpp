#include "ColliderSystem.h"
#include <raylib.h>

#include "Components/Collider.h"
#include "Components/Transform.h"
#include "Components/Trigger.h"

#include "Components/Velocity.h"
#include "Physics/CollisionLayer.h"
#include "Physics/HitInfo.h"
#include "Physics/QuadTree/Quadtree.h"

#include "Settings.h"
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

// Custom hash function
struct PairHash {
    std::size_t operator()(const std::pair<ecs::EntityID, ecs::EntityID>& p) const {
        int a = std::min(p.first, p.second);
        int b = std::max(p.first, p.second);
        return std::hash<int>()(a) ^ (std::hash<int>()(b) << 1);
    }
};

// Custom equality function
struct PairEqual {
    bool operator()(const std::pair<ecs::EntityID, ecs::EntityID>& lhs, const std::pair<ecs::EntityID, ecs::EntityID>& rhs) const {
        return (lhs.first == rhs.first && lhs.second == rhs.second) || (lhs.first == rhs.second && lhs.second == rhs.first);
    }
};

// Define the unordered_set with custom hash and equality
using PairSet = std::unordered_set<std::pair<ecs::EntityID, ecs::EntityID>, PairHash, PairEqual>;

static PairSet S_IGNORE_COLLISION;

ColliderSystem::ColliderSystem() {
    QUAD_TREE = makeDefaultQuadtree();
}

#ifndef NDEBUG
void ColliderSystem::drawDebug() {
    if (VIEW_COLLIDERS_MODE) {
        for (const auto [entityid, entity] : getEntities()) {
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

void ColliderSystem::setIsIgnoreCollision(ecs::Entity first, ecs::Entity second, bool ignore) {
    if (getEntities().contains(first.id()) && getEntities().contains(second.id())) {
        if (ignore) {
            S_IGNORE_COLLISION.emplace(first.id(), second.id());

        } else {
            S_IGNORE_COLLISION.erase({first.id(), second.id()});
        }
    }
#ifndef NDEBUG
    else {
        print("skipping call to setIsIgnoreCollision because one or both entities are not part of the ColliderSystem. Did you forget to activate an "
              "entity before calling this?");
    }
#endif
}

bool ColliderSystem::isIgnoreCollision(ecs::Entity first, ecs::Entity second) {
    return S_IGNORE_COLLISION.find({first.id(), second.id()}) != S_IGNORE_COLLISION.end();
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
        // things which can be pushed/carried should implicitly have related components added
        if (!entity.has<Momentum>()) {
            entity.add<Momentum>();
        }
        if (!entity.has<Velocity>()) {
            entity.add<Velocity>();
        }
    }
    QUAD_TREE.add(entity);
}

void ColliderSystem::onRemove(ecs::Entity entity) {
    QUAD_TREE.remove(entity);
    for (auto it = S_IGNORE_COLLISION.begin(); it != S_IGNORE_COLLISION.end();) {
        if (it->first == entity.id() || it->second == entity.id()) {
            it = S_IGNORE_COLLISION.erase(it);  // erase returns the next valid iterator
        } else {
            ++it;
        }
    }
}

void ColliderSystem::syncColliders() {
    for (const auto [entityid, entity] : getEntities()) {
        Transform& trans = entity.get<Transform>();
        if (!trans.isDirty) {
            continue;
        }
        const bool isManuallyMoved = trans.isManuallyMoved;
        trans.isManuallyMoved = false;

        auto& collider = entity.get<Collider>();
        if (isManuallyMoved) {
            // Sync collider position without checking collision
            if (collider.getShape().getPosition() != trans.apply(collider.getOffset())) {
                updatePosition(entity, collider.getShapeMutable(), trans, collider.getOffset());
            }

        } else {
            // Move collider within physics engine
            const Vector2i targetColliderPosition = trans.apply(collider.getOffset());
            if (collider.getShape().getPosition() != targetColliderPosition) {
                const Vector2f toMove = (targetColliderPosition - collider.getShape().getPosition()).as<f32>();

                // IsManualMove=true, so transform and QuadTree are synced automatically
                collider.move(toMove, nullptr, false, true);
            }
        }
    }
}

}  // namespace whal

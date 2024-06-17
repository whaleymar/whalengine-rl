#include "CollisionManager.h"
#include <raylib.h>

#include "ECS/Collision.h"
#include "ECS/Draw.h"
#include "ECS/Systems/TagTrackers.h"
#include "ECS/Systems/TriggerSystem.h"
#include "ECS/TriggerZone.h"
#include "Game/Events.h"
#include "Physics/CollisionLayer.h"
#include "Systems/System.h"
#include "Util/Vector.h"

namespace whal {

CollisionManager* CollisionManager::instance() {
    static auto instance_ = System::world->registerSystem<CollisionManager>();
    return instance_;
}

void CollisionManager::update() {
    if (!mIsUpdateNeeded) {
        return;
    }

    // put colliders with callbacks first so they get priority in collision checks
    // RESEARCH there's definitely a smarter way to do this
    std::vector<Collider*> newColliderList;
    std::vector<Collider*> newCallbackColliderList;
    for (auto [entityid, entity] : getEntitiesRef()) {
        auto pCollider = &entity.get<Collider>();
        if (!LAYER_MATRIX.isPhysicsLayer(pCollider->getCollisionLayer())) {
            continue;
        }
        if (pCollider->getOnCollisionEnter() == nullptr) {
            newColliderList.push_back(pCollider);
        } else {
            newCallbackColliderList.push_back(pCollider);
        }
    }

    mPhysicsColliders.clear();
    mPhysicsColliders.reserve(newCallbackColliderList.size() + newColliderList.size());
    mPhysicsColliders.insert(mPhysicsColliders.end(), newCallbackColliderList.begin(), newCallbackColliderList.end());
    mPhysicsColliders.insert(mPhysicsColliders.end(), newColliderList.begin(), newColliderList.end());

    mIsUpdateNeeded = false;
}

void CollisionManager::onAdd(ecs::Entity entity) {
    auto pCollider = &entity.get<Collider>();
    if (LAYER_MATRIX.isPhysicsLayer(pCollider->getCollisionLayer())) {
        mPhysicsColliders.push_back(pCollider);
        if (pCollider->getOnCollisionEnter() != nullptr) {
            mIsUpdateNeeded = true;
        }
    }
    pCollider->setEntity(entity);
}

void CollisionManager::onRemove(ecs::Entity entity) {
    mIsUpdateNeeded = true;
}

#ifndef NDEBUG
void drawColliders() {
    // auto cameraPos = getCameraPositionPrecise();
    auto cameraPos = toFloatVec(getCameraPosition());
    for (const auto collider : CollisionManager::instance()->getPhysicsColliders()) {
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

}  // namespace whal

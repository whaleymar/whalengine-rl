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
    static auto instance_ = System::ecs->registerSystem<CollisionManager>();
    return instance_;
}

void CollisionManager::update() {
    if (!mIsUpdateNeeded) {
        return;
    }
    std::vector<Collider*> newColliderList;

    // TODO need to put things with callbacks in the front like i do for solids (or do somethign smarter I feel like i had a good idea the other day)
    for (auto [entityid, entity] : getEntitiesRef()) {
        auto pCollider = &entity.get<Collider>();
        if (LAYER_MATRIX.isPhysicsLayer(pCollider->getCollisionLayer())) {
            newColliderList.push_back(pCollider);
        }
    }

    mPhysicsColliders = std::move(newColliderList);
    mIsUpdateNeeded = false;
}

void CollisionManager::onAdd(ecs::Entity entity) {
    auto pCollider = &entity.get<Collider>();
    if (LAYER_MATRIX.isPhysicsLayer(pCollider->getCollisionLayer())) {
        mPhysicsColliders.push_back(pCollider);
    }
    pCollider->setEntity(entity);
}

void CollisionManager::onRemove(ecs::Entity entity) {
    mIsUpdateNeeded = true;
}

// void SolidsManager::update() {
//     if (!mIsUpdateNeeded) {
//         return;
//     }
//     // put colliders with callbacks first so they get priority in collision checks
//     std::vector<SolidCollider*> newSolids;
//     std::vector<SolidCollider*> newSolidsWithCallbacks;
//     for (auto& [entityid, entity] : getEntitiesRef()) {
//         auto pCollider = &entity.get<SolidCollider>();
//         if (pCollider->getOnCollisionEnter() == nullptr) {
//             newSolids.push_back(pCollider);
//         } else {
//             newSolidsWithCallbacks.push_back(pCollider);
//         }
//     }
//
//     mSolids.clear();
//     mSolids.reserve(newSolidsWithCallbacks.size() + newSolids.size());
//     mSolids.insert(mSolids.end(), newSolidsWithCallbacks.begin(), newSolidsWithCallbacks.end());
//     mSolids.insert(mSolids.end(), newSolids.begin(), newSolids.end());
//
//     mIsUpdateNeeded = false;
// }
//
// void SolidsManager::onAdd(ecs::Entity entity) {
//     SolidCollider* pCollider = &entity.get<SolidCollider>();
//     mSolids.push_back(pCollider);
//     pCollider->setEntity(entity);
//     if (pCollider->getOnCollisionEnter() != nullptr) {
//         mIsUpdateNeeded = true;
//     }
// }
//
// void SolidsManager::onRemove(ecs::Entity entity) {
//     mIsUpdateNeeded = true;
// }

#ifndef NDEBUG
void drawColliders() {
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

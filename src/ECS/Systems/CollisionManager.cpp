#include "CollisionManager.h"
#include <raylib.h>

#include "ECS/Collision.h"
#include "ECS/Draw.h"
#include "ECS/Systems/TagTrackers.h"
#include "ECS/Systems/TriggerSystem.h"
#include "ECS/TriggerZone.h"
#include "Game/Events.h"

namespace whal {

void ActorsManager::update() {
    if (!mIsUpdateNeeded) {
        return;
    }
    std::vector<ActorCollider*> newActorList;

    for (auto& [entityid, entity] : getEntitiesRef()) {
        auto pCollider = &entity.get<ActorCollider>();
        newActorList.push_back(pCollider);
    }

    mActors = std::move(newActorList);
    mIsUpdateNeeded = false;
}

void ActorsManager::onAdd(ecs::Entity entity) {
    auto pCollider = &entity.get<ActorCollider>();
    mActors.push_back(pCollider);
    pCollider->setEntity(entity);
}

void ActorsManager::onRemove(ecs::Entity entity) {
    mIsUpdateNeeded = true;
}

void SolidsManager::update() {
    if (!mIsUpdateNeeded) {
        return;
    }
    // put colliders with callbacks first so they get priority in collision checks
    std::vector<SolidCollider*> newSolids;
    std::vector<SolidCollider*> newSolidsWithCallbacks;
    for (auto& [entityid, entity] : getEntitiesRef()) {
        auto pCollider = &entity.get<SolidCollider>();
        if (pCollider->getOnCollisionEnter() == nullptr) {
            newSolids.push_back(pCollider);
        } else {
            newSolidsWithCallbacks.push_back(pCollider);
        }
    }

    mSolids.clear();
    mSolids.reserve(newSolidsWithCallbacks.size() + newSolids.size());
    mSolids.insert(mSolids.end(), newSolidsWithCallbacks.begin(), newSolidsWithCallbacks.end());
    mSolids.insert(mSolids.end(), newSolids.begin(), newSolids.end());

    mIsUpdateNeeded = false;
}

void SolidsManager::onAdd(ecs::Entity entity) {
    SolidCollider* pCollider = &entity.get<SolidCollider>();
    mSolids.push_back(pCollider);
    pCollider->setEntity(entity);
    if (pCollider->getOnCollisionEnter() != nullptr) {
        mIsUpdateNeeded = true;
    }
}

void SolidsManager::onRemove(ecs::Entity entity) {
    mIsUpdateNeeded = true;
}

void SemiSolidsManager::update() {
    if (!mIsUpdateNeeded) {
        return;
    }
    // put colliders with callbacks first so they get priority in collision checks
    std::vector<SemiSolidCollider*> newSemis;
    std::vector<SemiSolidCollider*> newSemisWithCallbacks;
    for (auto& [entityid, entity] : getEntitiesRef()) {
        auto pCollider = &entity.get<SemiSolidCollider>();
        if (pCollider->getOnCollisionEnter() == nullptr) {
            newSemis.push_back(pCollider);
        } else {
            newSemisWithCallbacks.push_back(pCollider);
        }
    }

    mSemiSolids.clear();
    mSemiSolids.reserve(newSemisWithCallbacks.size() + newSemis.size());
    mSemiSolids.insert(mSemiSolids.end(), newSemisWithCallbacks.begin(), newSemisWithCallbacks.end());
    mSemiSolids.insert(mSemiSolids.end(), newSemis.begin(), newSemis.end());

    mIsUpdateNeeded = false;
}

void SemiSolidsManager::onAdd(ecs::Entity entity) {
    SemiSolidCollider* pCollider = &entity.get<SemiSolidCollider>();
    mSemiSolids.push_back(pCollider);
    pCollider->setEntity(entity);
    if (pCollider->getOnCollisionEnter() != nullptr) {
        mIsUpdateNeeded = true;
    }
}

void SemiSolidsManager::onRemove(ecs::Entity entity) {
    mIsUpdateNeeded = true;
}

#ifndef NDEBUG

void drawCollider(Vector2f cameraPos, const AABB& aabb, const Color color) {
    // Vector2f position(aabb.left(), aabb.top());
    Vector2f position(aabb.left(), aabb.bottom());
    Vector2f size = Vector2f(aabb.half.x(), aabb.half.y()) * 2;

    Vector2f dstPosition = {position.x() - cameraPos.x(), -1 * (position.y() + cameraPos.y())};
    DrawRectangleLines(dstPosition.x(), dstPosition.y(), size.x(), size.y(), color);
}

void drawColliders() {
    auto cameraPos = toFloatVec(getCameraPosition());
    for (const auto& collider : ActorsManager::instance()->getAllActors()) {
        drawCollider(cameraPos, collider->getCollider(), Colors::Magenta);
    }
    for (const auto& collider : SolidsManager::instance()->getAllSolids()) {
        drawCollider(cameraPos, collider->getCollider(), RED);
    }
    for (const auto& collider : SemiSolidsManager::instance()->getAllSemiSolids()) {
        drawCollider(cameraPos, collider->getCollider(), Colors::Pink);
    }
    for (const auto& [entityid, entity] : TriggerSystem::getEntitiesRef()) {
        drawCollider(cameraPos, entity.get<TriggerZone>(), Colors::Emerald);
    }
}
#endif

}  // namespace whal

#include "CollisionManager.h"

#include "ECS/Collision.h"
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
    // #ifndef NDEBUG
    //     entity.get<ActorCollider>().getColliderMut().vao.initArray();
    //     entity.get<ActorCollider>().getColliderMut().vbo.initBuffer();
    // #endif
}

void ActorsManager::onRemove(ecs::Entity entity) {
    mIsUpdateNeeded = true;
    // #ifndef NDEBUG
    //     entity.get<ActorCollider>().getColliderMut().vao.freeArray();
    //     entity.get<ActorCollider>().getColliderMut().vbo.freeBuffer();
    // #endif
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
    // #ifndef NDEBUG
    //     entity.get<SolidCollider>().getColliderMut().vao.initArray();
    //     entity.get<SolidCollider>().getColliderMut().vbo.initBuffer();
    // #endif
}

void SolidsManager::onRemove(ecs::Entity entity) {
    mIsUpdateNeeded = true;
    // #ifndef NDEBUG
    //     entity.get<SolidCollider>().getColliderMut().vao.freeArray();
    //     entity.get<SolidCollider>().getColliderMut().vbo.freeBuffer();
    // #endif
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
    // #ifndef NDEBUG
    //     entity.get<SemiSolidCollider>().getColliderMut().vao.initArray();
    //     entity.get<SemiSolidCollider>().getColliderMut().vbo.initBuffer();
    // #endif
}

void SemiSolidsManager::onRemove(ecs::Entity entity) {
    mIsUpdateNeeded = true;
    // #ifndef NDEBUG
    //     entity.get<SemiSolidCollider>().getColliderMut().vao.freeArray();
    //     entity.get<SemiSolidCollider>().getColliderMut().vbo.freeBuffer();
    // #endif
}

// #ifndef NDEBUG

// void drawCollider(ShaderProgram program, const IUseCollision* collider, const RGB color) {
// void drawCollider(ShaderProgram program, const AABB& aabb, const RGB color) {
// const AABB& aabb = collider->getCollider();

// Vector2f floatPos(aabb.left(), aabb.top());
// glUniform2fv(program.drawOffsetUniform, 1, floatPos.e);
//
// glm::mat4 transMatrix(1.0f);
// transMatrix = glm::rotate(transMatrix, glm::radians(0.0f), glm::vec3(0.0, 0.0, 1.0));
// glUniformMatrix4fv(program.transformUniform, 1, GL_FALSE, glm::value_ptr(transMatrix));
//
// Vector2f size = Vector2f(aabb.half.x(), aabb.half.y()) * 2;
// glUniform2fv(program.sizeUniform, 1, size.e);
//
// aabb.vao.bind();
// auto vertices = MakeRectVerticesRGBUV(aabb.half.x() * 2, aabb.half.y() * 2, Depth::Debug, color);
// aabb.vbo.buffer(vertices.data(), vertices.size() * sizeof(float));
//
// updateShaderVars(program);
// glDrawArrays(GL_TRIANGLE_STRIP, 0, N_VERTS_RECT);
// }

// void drawColliders() {
// auto program = GLResourceManager::getInstance().getProgram(ShaderType::Debug);
// program.useProgram();
//
// auto cameraPosF = toFloatVec(getCameraPosition());
// glUniform2fv(program.cameraPositionUniform, 1, cameraPosF.e);
//
// for (const auto& collider : ActorsManager::getInstance()->getAllActors()) {
//     drawCollider(program, collider->getCollider(), Color::MAGENTA);
// }
// for (const auto& collider : SolidsManager::getInstance()->getAllSolids()) {
//     drawCollider(program, collider->getCollider(), Color::RED);
// }
// for (const auto& collider : SemiSolidsManager::getInstance()->getAllSemiSolids()) {
//     drawCollider(program, collider->getCollider(), Color::PINK);
// }
// for (const auto& [entityid, entity] : TriggerSystem::getEntitiesRef()) {
//     drawCollider(program, entity.get<TriggerZone>(), Color::EMERALD);
// }
// }
// #endif

}  // namespace whal

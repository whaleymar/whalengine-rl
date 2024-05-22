#pragma once

#include "Systems/System.h"
#include "whalECS/src/ECS.h"

namespace whal {

class ActorCollider;
class SolidCollider;
class SemiSolidCollider;

class ActorsManager : public ecs::ISystem<ActorCollider> {
public:
    static std::shared_ptr<ActorsManager> instance() {
        static std::shared_ptr<ActorsManager> instance_ = System::ecs->registerSystem<ActorsManager>();
        return instance_;
    }

    void update() override;
    void onAdd(ecs::Entity entity) override;
    void onRemove(ecs::Entity entity) override;

    const std::vector<ActorCollider*>& getAllActors() const { return mActors; }

private:
    std::vector<ActorCollider*> mActors;
    bool mIsUpdateNeeded = false;
};

class SolidsManager : public ecs::ISystem<SolidCollider> {
public:
    static std::shared_ptr<SolidsManager> instance() {
        static std::shared_ptr<SolidsManager> instance_ = System::ecs->registerSystem<SolidsManager>();
        return instance_;
    }

    void update() override;
    void onAdd(ecs::Entity entity) override;
    void onRemove(ecs::Entity entity) override;
    void setUpdateNeeded() { mIsUpdateNeeded = true; }

    const std::vector<SolidCollider*>& getAllSolids() const { return mSolids; }

private:
    std::vector<SolidCollider*> mSolids;
    size_t mNumCallbackColliders = 0;
    bool mIsUpdateNeeded = false;
};

class SemiSolidsManager : public ecs::ISystem<SemiSolidCollider> {
public:
    static std::shared_ptr<SemiSolidsManager> instance() {
        static std::shared_ptr<SemiSolidsManager> instance_ = System::ecs->registerSystem<SemiSolidsManager>();
        return instance_;
    }

    void update() override;
    void onAdd(ecs::Entity entity) override;
    void onRemove(ecs::Entity entity) override;
    void setUpdateNeeded() { mIsUpdateNeeded = true; }

    const std::vector<SemiSolidCollider*> getAllSemiSolids() const { return mSemiSolids; }

private:
    std::vector<SemiSolidCollider*> mSemiSolids;
    size_t mNumCallbackColliders = 0;
    bool mIsUpdateNeeded = false;
};

#ifndef NDEBUG
void drawColliders();
#endif

}  // namespace whal

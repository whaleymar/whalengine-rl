#pragma once

#include "whalECS/src/ECS.h"

namespace whal {

class Collider;

// RESEARCH may be better to store actor/semisolid/solid pointers in separate lists?
class CollisionManager : public ecs::ISystem<Collider>, public ecs::IMonitorSystem {
public:
    static CollisionManager* instance();

    void update();
    void onAdd(ecs::Entity entity) override;
    void onRemove(ecs::Entity entity) override;
    void setUpdateNeeded() { mIsUpdateNeeded = true; }

    const std::vector<Collider*> getPhysicsColliders() const { return mPhysicsColliders; }

private:
    std::vector<Collider*> mPhysicsColliders;
    bool mIsUpdateNeeded = false;
};

#ifndef NDEBUG
void drawColliders();
#endif

class QuadTreeSystem : public ecs::ISystem<Collider>, public ecs::IMonitorSystem {
public:
    void update();
    void onAdd(ecs::Entity entity) override;
    void onRemove(ecs::Entity entity) override;

private:
};

}  // namespace whal

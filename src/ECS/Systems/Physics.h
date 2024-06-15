#pragma once

#include <functional>

#include "Systems/Event.h"
#include "whalECS/src/ECS.h"

#include "Util/Types.h"

namespace whal {

struct Transform2D;
struct Velocity;
struct HitInfo;

inline constexpr f32 TERMINAL_VELOCITY_Y = -160;

class PhysicsSystem : public ecs::ISystem<Transform2D, Velocity> {
    using CallbackMap = std::unordered_map<ecs::Entity, std::vector<std::pair<ecs::Entity, std::function<void()>>>, ecs::EntityHash>;

public:
    PhysicsSystem();
    void update() override;

    static CallbackMap& getCollisionCallbackQueue() { return mCollisionCallbackQueue; }

private:
    whal::EventListener<ecs::Entity, HitInfo> mCollisionListener;
    inline static CallbackMap mCollisionCallbackQueue;
};

}  // namespace whal

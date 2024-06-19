#pragma once

#include <functional>

#include "Events/Events.h"
#include "Physics/HitInfo.h"
#include "Systems/System.h"
#include "whalECS/src/ECS.h"

#include "Util/Types.h"

namespace whal {

struct Transform2D;
struct Velocity;
struct HitInfo;

inline constexpr f32 TERMINAL_VELOCITY_Y = -160;

class PhysicsSystem : public ecs::ISystem<Transform2D, Velocity>, public IListen<CollisionEvent, ecs::Entity, HitInfo> {
    using CallbackMap = std::unordered_map<ecs::Entity, std::vector<std::pair<ecs::Entity, std::function<void()>>>, ecs::EntityHash>;

public:
    void update() override;
    void onEvent(ecs::Entity, HitInfo) override;

    static CallbackMap& getCollisionCallbackQueue() { return mCollisionCallbackQueue; }

private:
    inline static CallbackMap mCollisionCallbackQueue;
};

}  // namespace whal

#pragma once

#include "Settings.h"
#include "Systems/Event.h"
#include "Util/Vector.h"
#include "whalECS/src/ECS.h"

// TODO game specific components/systems should go in another folder

namespace whal {
struct Transform2D;
}

struct Blaster {
    f32 projectileSpeed = 160;  // same as terminal velocity
    f32 shotKnockback = 50;
    f32 projectileLifetimeSeconds = 3.5;
    f32 explosionRadius = FPIXELS_PER_TILE * 1.5;
};

// TODO should require a generic PlayerControl component too, and can have a separate system for NPCs -- see note in Blaster.cpp
class ProjectileSystem : public whal::ecs::ISystem<Blaster, whal::Transform2D> {
public:
    ProjectileSystem();

private:
    whal::EventListener<whal::Vector2i> mBlasterEventListener;
};

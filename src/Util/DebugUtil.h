#pragma once

// Declares entity.has<> and entity.get<> implementations so they can be used in GDB

#ifndef NDEBUG

#include "Components/Animator.h"
#include "Components/Camera.h"
#include "Components/Collision.h"
#include "Components/Draw.h"
#include "Components/Lifetime.h"
#include "Components/Light.h"
#include "Components/Name.h"
#include "Components/ParticleEmitter.h"
#include "Components/PlayerControl.h"
#include "Components/RailsControl.h"
#include "Components/Relationships.h"
#include "Components/RigidBody.h"
#include "Components/Tags.h"
#include "Components/Transform.h"
#include "Components/TriggerZone.h"
#include "Components/Velocity.h"

namespace whal::ecs {
class Entity;
}
#define DECLARE_COMPONENT(Type)                                                                                                                      \
    template bool whal::ecs::Entity::has<Type>() const;                                                                                              \
    template Type& whal::ecs::Entity::get<Type>() const;

namespace whal {}  // namespace whal

DECLARE_COMPONENT(whal::Transform);
DECLARE_COMPONENT(whal::Velocity);
DECLARE_COMPONENT(whal::AngularVelocity);
DECLARE_COMPONENT(whal::RailsControl);
DECLARE_COMPONENT(whal::Collider);
DECLARE_COMPONENT(whal::Trigger);
DECLARE_COMPONENT(whal::RigidBody);
DECLARE_COMPONENT(whal::PlayerControl);
DECLARE_COMPONENT(whal::Jumper);
DECLARE_COMPONENT(whal::Follow);
DECLARE_COMPONENT(whal::Attach);
DECLARE_COMPONENT(whal::PointLight);
DECLARE_COMPONENT(whal::BoxLight);
DECLARE_COMPONENT(whal::ShadowLight);
DECLARE_COMPONENT(whal::Lifetime);
DECLARE_COMPONENT(whal::DrawText);
DECLARE_COMPONENT(whal::ParticleEmitter);
DECLARE_COMPONENT(whal::Name);
DECLARE_COMPONENT(whal::Animator);
DECLARE_COMPONENT(whal::Orbit);
DECLARE_COMPONENT(whal::Player);
DECLARE_COMPONENT(whal::Camera);
DECLARE_COMPONENT(whal::AudioListener);
DECLARE_COMPONENT(whal::Particle);
DECLARE_COMPONENT(whal::Invisible);

#endif

#ifndef NDEBUG
#include <sstream>
#define DBG_ASSERT(cond, msg)                                                                                                                        \
    do {                                                                                                                                             \
        if (!(cond)) {                                                                                                                               \
            std::ostringstream str;                                                                                                                  \
            str << msg;                                                                                                                              \
            std::cerr << str.str() << std::endl;                                                                                                     \
            std::abort();                                                                                                                            \
        }                                                                                                                                            \
    } while (0)
#else
#define DBG_ASSERT(cond, msg)                                                                                                                        \
    do {                                                                                                                                             \
    } while (0)
#endif

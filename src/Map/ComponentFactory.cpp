#include "ComponentFactory.h"

#include "Components/Animator.h"
#include "Components/Draw.h"
#include "Components/Lifetime.h"
#include "Components/Light.h"
#include "Components/MonoBehavior.h"
#include "Components/ParticleEmitter.h"
#include "Components/PlayerControl.h"
#include "Components/RigidBody.h"
#include "Components/Transform.h"
#include "Components/Trigger.h"
#include "Components/Velocity.h"
#include "Map/Tiled.h"
#include "Serializer.h"
#include "Sys/System.h"

namespace whal {

template <typename T>
static ecs::Serialize defaultSerdeImpl() {
    return {
        .ser = [](ecs::Entity e) -> std::string { return rfl::json::write(e.get<T>()); },
        .de =
            [](ecs::Entity e, std::string data) {
                auto dataOpt = rfl::json::read<T>(data);
                if (dataOpt) {
                    e.add<T>(*dataOpt);
                } else {
                    print("Error parsing component: ", dataOpt.error()->what());
                    e.add<T>();
                }
            },
    };
}

template <typename T>
ecs::Serialize tagSerdeImpl() {
    return {
        .ser = nullptr,
        .de = [](ecs::Entity e, std::string data) { e.add<T>(); },
    };
}

template <typename T>
void addDefault() {
    ecs::Entity e = World.component<T>();
    if (e.has<ecs::internal::Tag>()) {
        e.add<ecs::Serialize>(tagSerdeImpl<T>());
    } else {
        e.add<ecs::Serialize>(defaultSerdeImpl<T>());
    }
}

// TODO replace current Static-Initialization pattern with this!
// I'm honestly fine with this pattern over inheriting ISerialize like I've been doing.
// It'll help reduce compile times across the whole project, and adding serialization definitions here really isn't much work.
// I can do a similar pattern for loading data from Tiled (separate component)

// For a lot of components, I also *don't* want to serialize the whole thing.
// - E.g. Sprite/Animator I would just want a string or two.
// - I can use non-default serde implementations for those
void initEcsSerializer() {
    // TODO gamecomponents? maybe put this file in game project
    addDefault<Transform>();
    addDefault<Sprite>();
    addDefault<DrawRect>();
    addDefault<DrawStraightLine>();
    addDefault<DrawText>();
    addDefault<DrawBezierQuad>();
    addDefault<SpriteOutline>();
    // addDefault<Animator>(); // complex class (i wouldn't want to default-serialize this anyway, probably just the string name + brain callback
    // name?)
    addDefault<Velocity>();
    addDefault<PlayerControl>();
    // addDefault<Collider>(); // complex class
    // addDefault<Wiggle>(); // has callback
    addDefault<PointLight>();
    addDefault<BoxLight>();
    addDefault<ShadowLight>();
    // addDefault<Lifetime>(); // has callback
    addDefault<ParticleEmitter>();
    addDefault<RigidBody>();
    // addDefault<TileMapLayer>(); // don't want default serializer
    // addDefault<Trigger>(); // has callbacks
    // addDefault<MonoBehavior>();
}

}  // namespace whal

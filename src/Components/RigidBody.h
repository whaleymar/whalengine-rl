#pragma once

#include "Map/ComponentFactory.h"
#include "Map/Tiled.h"
#include "Physics/Material.h"
#include "Util/Types.h"
#include "Util/Vector.h"

namespace whal {

struct RigidBody : ISerialize<RigidBody, ComponentFactory> {
    RigidBody() = default;
    RigidBody(Vector2f frictionMult) : frictionMultiplier(frictionMult) {}

    void setGrounded(WorldMaterial material);
    void setNotGrounded();

    Vector2f frictionMultiplier = {1.0, 1.0};  // x is ground, y is air
    Vector2f momentumMultiplier = {1.0, 0.5};
    f32 gravityMultiplier = 1.0;

    // automatically managed:
    s32 framesSinceLanding = 0;
    WorldMaterial groundMaterial = WorldMaterial::None;
    bool isLanding = false;
    bool isGrounded = false;

    static void loadImpl(ecs::Entity entity, void* data) {
        const LoadContext& ctx = *static_cast<LoadContext*>(data);
        RigidBody rb = entity.has<RigidBody>() ? entity.get<RigidBody>() : RigidBody{};

        tryReadVector2f(ctx.values, "momentumMultiplierX", "momentumMultiplierY", &rb.momentumMultiplier);
        tryReadVector2f(ctx.values, "frictionGround", "frictionAir", &rb.frictionMultiplier);
        tryReadFloat(ctx.values, "gravityMultiplier", &rb.gravityMultiplier);

        entity.add(rb);
    }
};

}  // namespace whal

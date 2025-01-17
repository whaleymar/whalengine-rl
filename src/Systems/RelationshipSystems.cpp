#include "RelationshipSystems.h"

#include "Components/Relationships.h"
#include "Components/Transform.h"
#include "Components/Velocity.h"
#include "Events/Events.h"
#include "Util/MathUtil.h"
#include "Util/Vector.h"

namespace whal {

// RESEARCH use collider.move if the entity has a collider? Seems like it would be glitchy if a collision does happen
void OrbitSystem::update() {
    const f32 dt = Time.dt();
    for (auto [entityid, entity] : getEntities()) {
        auto& trans = entity.get<Transform>();
        Orbit& orbit = entity.get<Orbit>();
        const ecs::Entity targetEntity(orbit.targetID);
        if (!orbit.isTargetInitialized) {
            orbit.initTarget(entity);
        }
        const Vector2f orbitTarget = targetEntity.get<Transform>().position + orbit.targetOffset.as<f32>();

        // if we get the current angle and add to that, it has this cool "follow if target moving, orbit if target is still" effect, but not sure if
        // that's useful for anything
        // const auto delta = trans.position - orbitTarget;
        // f32 angle = delta.isZero() ? 0.0f : getAngle(delta.as<f32>());

        // multiply by -1 so rotations are clockwise by default
        const f32 toAdd = -1.0f * 360.0f * orbit.rotationsPerSecond * dt;
        orbit.currentAngle += toAdd;

        // RESEARCH bool param so that entity rotates in sync with orbit? (tidal lock)
        const Vector2f unit = Vector2f::fromAngle(orbit.currentAngle);
        trans.setPosition((unit * static_cast<f32>(orbit.radius)) + orbitTarget + (orbit.selfOffset.as<f32>() * unit), entity);
    }
}

void FollowSystem::update() {
    // i nuked this because i wrote it when this engine was a baby and it didn't even work
    // for (auto [entityid, entity] : getEntities()) {
    //     Transform trans = entity.get<Transform>();
    //     auto& follow = entity.get<Follow>();
    //     if (!follow.isTargetInitialized) {
    //         follow.initTarget(entity);
    //     }
    // }
}

void FollowSystem::onRemove(ecs::Entity entity) {
    // reset any lingering effects on velocity
    if (entity.has<Velocity>()) {
        entity.set(Velocity());
    }
}

// if the target of an entity's Follow component dies, remove the follow component.
void FollowSystem::onEvent(evt::Death, ecs::Entity killedEntity) {
    std::vector<ecs::Entity> toRemove;
    for (auto& [entityid, entity] : FollowSystem::getEntities()) {
        if (entity.get<Follow>().targetEntityID == killedEntity.id()) {
            toRemove.push_back(entity);
        }
    }

    for (auto entity : toRemove) {
        entity.remove<Follow>();
    }
}

}  // namespace whal

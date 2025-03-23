#include "Particle.h"

#include <raylib.h>

#include "Components/Draw.h"
#include "Components/Lifetime.h"
#include "Components/Tags.h"
#include "Components/Transform.h"
#include "Components/Velocity.h"

#include "Physics/MaterialData.h"
#include "Physics/Shapes.h"

#include "Settings.h"
#include "Sys/System.h"
#include "Util/Vector.h"

namespace whal {

constexpr f32 BURST_SPREAD_ANGLE = 45.0f;

ecs::Entity createParticle(Vector2i worldPosition, WorldMaterial material, Depth depth, f32 lifetimeMultiplier) {
    return createParticle(worldPosition, MaterialData::get(material), depth, lifetimeMultiplier);
}

ecs::Entity createParticle(Vector2i worldPosition, const MaterialData& materialData, Depth depth, f32 lifetimeMultiplier) {
    auto particle = World.entity(false);
    if (!particle.isValid()) {
        return particle;
    }
    auto _ = ecs::DeferActivate(particle);

    Transform trans = Transform::world(worldPosition);
    trans.depth = depth;
    particle.set(trans);
    addParticleComponents(particle, materialData, lifetimeMultiplier);

    return particle;
}

void addParticleComponents(ecs::Entity particle, const MaterialData& materialData, f32 lifetimeMultiplier) {
    particle.setName("particle");
    particle.add<Particle>();
    particle.add<Velocity>();

    const Color color = materialData.getColor();
    if (materialData.particleShape == DrawTag::Line) {
        particle.add(DrawStraightLine{
            .length = 3,
            .color = color,
            .thickness = 1.0,
            .isRotateAboutCenter = true,
        });
        materialData.addComponents<DrawStraightLine>(particle, 1, color, lifetimeMultiplier);
    } else {
        particle.add(DrawRect::create(color, Vector2i(1, 1)));
        materialData.addComponents<DrawRect>(particle, 1, color, lifetimeMultiplier);
    }
}

void particleBurst(Transform transform, Direction direction, WorldMaterial material, s32 count, Depth depth, f32 lifetimeMultiplier, f32 minSpeed,
                   f32 maxSpeed) {
    particleBurst(transform, direction, MaterialData::get(material), count, depth, lifetimeMultiplier, minSpeed, maxSpeed);
}

void particleBurst(Transform transform, Direction direction, const MaterialData& material, s32 count, Depth depth, f32 lifetimeMultiplier,
                   f32 minSpeed, f32 maxSpeed) {
    f32 angle = directionToAngle(direction);
    const AABB spawnZone(transform, {PIXELS_PER_TILE / 2, 1}, Vector2i());

    for (s32 i = 0; i < count; i++) {
        const f32 locationSampleX = (Rng.uniform() - 0.5) * 2;
        const f32 locationSampleY = (Rng.uniform() - 0.5) * 2;
        const s32 spawnOffsetX = (std::roundf((f32)spawnZone.getHalf().x * locationSampleX));
        const s32 spawnOffsetY = (std::roundf((f32)spawnZone.getHalf().y * locationSampleY));
        const Vector2i spawnLocation = spawnZone.getPosition() + Vector2i(spawnOffsetX, spawnOffsetY);
        if (direction == Direction::Neutral) {
            angle = Rng.range(0, 360);
        }
        const f32 finalAngle = angle + BURST_SPREAD_ANGLE * ((Rng.uniform() - 0.5) * 2);
        const f32 finalSpeed = std::lerp(minSpeed, maxSpeed, Rng.uniform());

        ecs::Entity particle = createParticle(spawnLocation, material, depth, lifetimeMultiplier);
        if (!particle.isValid()) {
            continue;
        }

        particle.set(Velocity::from(Vector2f::fromAngleFast(finalAngle) * finalSpeed));
    }
}

}  // namespace whal

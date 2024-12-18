#include "Particle.h"

#include <raylib.h>

#include "Components/Draw.h"
#include "Components/Lifetime.h"
#include "Components/Name.h"
#include "Components/Tags.h"
#include "Components/Transform.h"
#include "Components/Velocity.h"

#include "Physics/MaterialData.h"
#include "Physics/Shapes.h"

#include "Settings.h"
#include "Sys/System.h"
#include "Util/Vector.h"

namespace whal {

constexpr f32 MIN_SPEED_BURST = 5.0f;
constexpr f32 MAX_SPEED_BURST = 20.0f;
constexpr f32 BURST_SPREAD_ANGLE = 45.0f;

ecs::Entity createParticle(Vector2i worldPosition, WorldMaterial material, Depth depth, f32 lifetimeMultiplier) {
    const MaterialData materialData = MaterialData::get(material);
    const Color color = materialData.getColor();

    auto particle = World.entity(false);
    if (!particle.isValid()) {
        return particle;
    }
    auto _ = ecs::DeferActivate(particle);

    Transform trans = Transform::world(worldPosition);
    trans.depth = depth;
    particle.set(trans);
    particle.add(Name("particle"));
    particle.add<Particle>();
    particle.add<Velocity>();

    if (materialData.particleShape == DrawTag::Line) {
        particle.add(DrawStraightLine{
            .length = 3,
            .color = color,
            .thickness = 1.0,
            .isRotateAboutCenter = true,
        });
        materialData.addComponents<DrawStraightLine>(particle, 1, color, lifetimeMultiplier);
        particle.add(AngularVelocity{.rotationsPerSecond = Rng.range(0.25f, 2.0f)});
    } else {
        particle.add(DrawRect::create(color, Vector2i(1, 1)));
        materialData.addComponents<DrawRect>(particle, 1, color, lifetimeMultiplier);
    }

    return particle;
}

void particleBurst(Transform transform, Direction direction, WorldMaterial material, s32 count, Depth depth, f32 lifetimeMultiplier,
                   f32 speedMultiplier) {
    const f32 angle = directionToAngle(direction);
    const AABB spawnZone(transform, {PIXELS_PER_TILE / 2, 1}, Vector2i());

    if (material == WorldMaterial::Grass) {
        count /= 2;
    }

    for (s32 i = 0; i < count; i++) {
        const f32 locationSampleX = (Rng.uniform() - 0.5) * 2;
        const f32 locationSampleY = (Rng.uniform() - 0.5) * 2;
        const s32 spawnOffsetX = (std::roundf((f32)spawnZone.getHalf().x * locationSampleX));
        const s32 spawnOffsetY = (std::roundf((f32)spawnZone.getHalf().y * locationSampleY));
        const Vector2i spawnLocation = spawnZone.getPosition() + Vector2i(spawnOffsetX, spawnOffsetY);
        const f32 finalAngle = angle + BURST_SPREAD_ANGLE * ((Rng.uniform() - 0.5) * 2);
        const f32 finalSpeed = std::lerp(MIN_SPEED_BURST, MAX_SPEED_BURST, Rng.uniform()) * speedMultiplier;

        ecs::Entity particle = createParticle(spawnLocation, material, depth, lifetimeMultiplier);
        if (!particle.isValid()) {
            continue;
        }

        particle.set(Velocity(Vector2f::fromAngleFast(finalAngle) * finalSpeed));
        if (particle.has<AngularVelocity>()) {
            particle.set(AngularVelocity::fromSecondsPerRotation(Rng.range(1.5f, 2.5f)));
        }
    }
}

}  // namespace whal

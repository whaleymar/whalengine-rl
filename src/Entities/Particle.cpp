#include "Particle.h"

#include <raylib.h>

#include "Components/Draw.h"
#include "Components/Lifetime.h"
#include "Components/Light.h"
#include "Components/Name.h"
#include "Components/Tags.h"
#include "Components/Transform.h"
#include "Components/Velocity.h"

#include "Physics/Material.h"
#include "Physics/Shapes.h"

#include "Settings.h"
#include "Sys/System.h"
#include "Util/Vector.h"

namespace whal {

constexpr f32 MIN_SPEED_BURST = 5.0f;
constexpr f32 MAX_SPEED_BURST = 20.0f;
constexpr f32 BURST_SPREAD_ANGLE = 45.0f;

static Expected<ecs::Entity> createParticleBase(Transform2D transform, Color color, f32 lifetime) {
    auto expected = System::world.entity(false);
    if (!expected.isExpected()) {
        return expected;
    }
    auto particle = expected.value();

    particle.add(transform);
    particle.add(Name("particle"));
    particle.add(PrecisePosition::fromTrans(transform));
    particle.add(Lifetime(lifetime));
    particle.add<Particle>();

    return particle;
}

Expected<ecs::Entity> createParticle(Transform2D transform, WorldMaterial material, Depth depth, f32 lifetimeMultiplier) {
    const MaterialData materialData = MaterialData::get(material);
    const Color color = materialData.getColor();

    auto expected = System::world.entity(false);
    if (!expected.isExpected()) {
        return expected;
    }
    auto particle = expected.value();
    auto _ = ecs::DeferActivate(particle);

    particle.add(transform);
    particle.add(Name("particle"));
    particle.add(PrecisePosition::fromTrans(transform));
    particle.add<Particle>();
    particle.add<Velocity>();
    materialData.addComponents(particle, 1, color, lifetimeMultiplier);

    // TextureID texID = materialData.isFlagSet(MaterialData::RadianceFlag) ? TextureID::LayerBloom : TextureID::LayerNormal;
    TextureID texID = TextureID::LayerNormal;

    if (materialData.particleShape == Draw::DrawTag::Line) {
        particle.add(Draw(DrawStraightLine(3, color, 1.0, 1.0, 1.0, depth, true), texID));
        particle.add<AngularVelocity>();
    } else {
        particle.add(Draw(DrawRect(color, Vector2i(1, 1), depth), texID));
    }

    return particle;
}

Expected<ecs::Entity> createParticleSprite(Transform2D transform, Color color, f32 lifetime, bool fullRadiance, Depth depth) {
    auto expected = createParticleBase(transform, color, lifetime);
    if (!expected.isExpected()) {
        return expected;
    }
    auto _ = ecs::DeferActivate(expected.value());
    auto particle = expected.value();

    auto frame = TextureManager::instance().getTextureAtlas(TEXNAME_SPRITE).getFrame("actor/mana-gauge1");
    particle.add(Draw(Sprite(depth, *frame)));

    s32 radius = TEXELS_PER_TILE * 1;
    particle.add(PointLight{radius, 0, color});
    if (fullRadiance) {
        particle.add(Radiance{radius / 2, 0, color});

    } else {
        particle.add(Radiance{radius / 2, 0, Color(color.r, color.g, color.b, color.a / 2)});
    }

    return particle;
}

void particleBurst(Transform2D transform, Direction direction, WorldMaterial material, s32 count, Depth depth, f32 lifetimeMultiplier,
                   f32 speedMultiplier) {
    const f32 angle = directionToAngle(direction);
    const AABB spawnZone(transform, {PIXELS_PER_TILE / 2, 1});

    if (material == WorldMaterial::Grass) {
        count /= 2;
    }

    for (s32 i = 0; i < count; i++) {
        const f32 locationSampleX = (System::rng.uniform() - 0.5) * 2;
        const f32 locationSampleY = (System::rng.uniform() - 0.5) * 2;
        const s32 spawnOffsetX = (std::roundf((f32)spawnZone.getHalf().x * locationSampleX));
        const s32 spawnOffsetY = (std::roundf((f32)spawnZone.getHalf().y * locationSampleY));
        const Vector2i spawnLocation = spawnZone.getPosition() + Vector2i(spawnOffsetX, spawnOffsetY);
        const f32 finalAngle = angle + BURST_SPREAD_ANGLE * ((System::rng.uniform() - 0.5) * 2);
        const f32 finalSpeed = std::lerp(MIN_SPEED_BURST, MAX_SPEED_BURST, System::rng.uniform()) * speedMultiplier;

        ecs::Entity particle;
        auto eParticle = createParticle(Transform2D(spawnLocation), material, depth, lifetimeMultiplier);
        if (eParticle.isExpected()) {
            particle = eParticle.value();
        } else {
            continue;
        }

        particle.set(Velocity(angleToUnit(finalAngle) * finalSpeed));
        if (particle.has<AngularVelocity>()) {
            particle.set(AngularVelocity::fromSecondsPerRotation(System::rng.range(1.5f, 2.5f)));
        }
    }
}

}  // namespace whal

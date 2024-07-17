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

struct ParticleInfo {
    Color color;
    f32 lifetimeSeconds;
    f32 speedRatio;
};

ParticleInfo particleLookup(WorldMaterial material) {
    switch (material) {
    case WorldMaterial::None:
        return {WHITE, 1.0, 0.0};
    case WorldMaterial::Dirt:
        return {BROWN, 1.0, 0.0};
    case WorldMaterial::Rock:
        return {DARKGRAY, 1.0, 0.0};
    case WorldMaterial::Soft:
        return {WHITE, 1.0, 0.5};
    case WorldMaterial::Wood:
        return {BROWN, 1.0, 0.5};
    case WorldMaterial::Grass:
        return {DARKGREEN, 1.0, 1.0};
    case WorldMaterial::Metal:
        return {GRAY, 1.0, 0.0};
    case WorldMaterial::Water:
        return {DARKBLUE, 1.0, 0.25};
    case WorldMaterial::Rubber:
        return {BLACK, 1.0, 1.0};
    }
}

static Expected<ecs::Entity> createParticleBase(Transform2D transform, Color color, f32 lifetime) {
    auto expected = System::world->entity(false);
    if (!expected.isExpected()) {
        return expected;
    }
    auto particle = expected.value();

    particle.add(transform);
    particle.add(Name("particle"));
    particle.add(PrecisePosition::fromTrans(transform));
    particle.add(Lifetime(lifetime));
    particle.add(FadeOut(lifetime));
    particle.add<Particle>();

    return particle;
}

Expected<ecs::Entity> createParticle(Transform2D transform, Color color, f32 lifetime, Depth depth) {
    auto expected = createParticleBase(transform, color, lifetime);
    if (!expected.isExpected()) {
        return expected;
    }
    auto _ = ecs::DeferActivate(expected.value());
    auto particle = expected.value();

    particle.add(Draw(DrawRect(color, Vector2i(1, 1), depth)));

    return particle;
}

Expected<ecs::Entity> createParticleLight(Transform2D transform, Color color, f32 lifetime, bool fullRadiance, Depth depth) {
    auto expected = createParticleBase(transform, color, lifetime);
    if (!expected.isExpected()) {
        return expected;
    }
    auto _ = ecs::DeferActivate(expected.value());
    auto particle = expected.value();

    particle.add(Draw(DrawRect(color, Vector2i(1, 1), depth)));

    s32 radius = TEXELS_PER_TILE * 1;
    particle.add(PointLight{radius, 0, color});
    if (fullRadiance) {
        particle.add(Radiance{radius / 2, 0, color});

    } else {
        particle.add(Radiance{radius / 2, 0, Color(color.r, color.g, color.b, color.a / 2)});
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

void particleBurst(Transform2D transform, Direction direction, WorldMaterial material, s32 count, Depth depth) {
    const f32 angle = directionToAngle(direction);
    const AABB spawnZone(transform, {PIXELS_PER_TILE / 2, 1});

    for (s32 i = 0; i < count; i++) {
        const f32 locationSampleX = (System::rng.uniform() - 0.5) * 2;
        const f32 locationSampleY = (System::rng.uniform() - 0.5) * 2;
        const s32 spawnOffsetX = (std::roundf((f32)spawnZone.getHalf().x() * locationSampleX));
        const s32 spawnOffsetY = (std::roundf((f32)spawnZone.getHalf().y() * locationSampleY));
        const Vector2i spawnLocation = spawnZone.getPosition() + Vector2i(spawnOffsetX, spawnOffsetY);
        const f32 finalAngle = angle + BURST_SPREAD_ANGLE * ((System::rng.uniform() - 0.5) * 2);
        // const f32 finalSpeed = std::lerp(0.5 * MAX_SPEED_BURST, MAX_SPEED_BURST, System::rng.uniform());
        const ParticleInfo pInfo = particleLookup(material);
        const f32 finalSpeed = std::lerp(MIN_SPEED_BURST, MAX_SPEED_BURST, pInfo.speedRatio);

        ecs::Entity particle;
        auto eParticle = createParticle(Transform2D(spawnLocation), pInfo.color, pInfo.lifetimeSeconds, depth);
        if (eParticle.isExpected()) {
            particle = eParticle.value();
        } else {
            continue;
        }

        particle.add(Velocity(angleToUnit(finalAngle) * finalSpeed));

        // if ((emitter.settings & ParticleSetting::RigidBody) > 0) {
        //     particle.add(RigidBody({0.0f, 0.0f}));
        // }
        //
        // if ((emitter.settings & ParticleSetting::Collider) > 0) {
        //     particle.add(Collider::Actor(AABB(spawnLocation, {1, 1})));
        //     particle.get<Collider>().setMaterial(WorldMaterial::Rubber);
        // }
    }
}

}  // namespace whal

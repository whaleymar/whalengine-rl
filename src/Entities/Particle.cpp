#include "Particle.h"

#include "Components/Draw.h"
#include "Components/Lifetime.h"
#include "Components/Light.h"
#include "Components/Name.h"
#include "Components/Tags.h"
#include "Components/Transform.h"
#include "Settings.h"

#include "Sys/System.h"

namespace whal {

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

}  // namespace whal

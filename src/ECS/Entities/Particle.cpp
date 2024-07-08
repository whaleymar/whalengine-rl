#include "Particle.h"

#include "ECS/Draw.h"
#include "ECS/Lifetime.h"
#include "ECS/Light.h"
#include "ECS/Name.h"
#include "ECS/Tags.h"
#include "ECS/Transform.h"
#include "Settings.h"

#include "Systems/System.h"

namespace whal {

Expected<ecs::Entity> createParticle(Transform2D transform, Color color, f32 lifetime) {
    auto expected = System::world->entity(false);
    if (!expected.isExpected()) {
        return expected;
    }
    auto _ = ecs::DeferActivate(expected.value());
    auto particle = expected.value();

    particle.add(transform);
    particle.add(PrecisePosition::fromTrans(transform));
    particle.add(Draw(color, Vector2i(1, 1), Depth::Foreground1));
    particle.add(Lifetime(lifetime));
    particle.add<Particle>();

    return particle;
}

Expected<ecs::Entity> createParticleLight(Transform2D transform, Color color, f32 lifetime) {
    auto expected = System::world->entity(false);
    if (!expected.isExpected()) {
        return expected;
    }
    auto _ = ecs::DeferActivate(expected.value());
    auto particle = expected.value();

    particle.add(transform);
    particle.add(PrecisePosition::fromTrans(transform));
    particle.add(Name("particle"));
    particle.add(Draw(color, Vector2i(1, 1), Depth::Foreground1));
    particle.add(Lifetime(lifetime));
    particle.add<Particle>();

    s32 radius = TEXELS_PER_TILE * 1;
    particle.add(PointLight{radius, 0, color});
    particle.add(Radiance{radius / 2, 0, color});

    return particle;
}

}  // namespace whal

#include "Projectile.h"

#include "Components/Callback.h"
#include "Components/Light.h"
#include "Components/ParticleEmitter.h"
#include "Components/RigidBody.h"
#include "Entities/Particle.h"
#include "Game/Components/ProjectileInfo.h"
#include "Physics/CollisionLayer.h"
#include "Physics/Material.h"
#include "Physics/Shapes.h"
#include "Settings.h"
#include "whalECS/src/ECS.h"

#include "Components/AnimUtil.h"
#include "Components/Animator.h"
#include "Components/Collision.h"
#include "Components/Draw.h"
#include "Components/Lifetime.h"
#include "Components/Name.h"
#include "Components/Transform.h"
#include "Components/Velocity.h"
#include "Explosion.h"

#include "Sys/System.h"
#include "Util/MathUtil.h"

using namespace whal;

// skip explosion if we're colliding with the entity that shot us and the shot just occurred
static bool skipParentCollision(ecs::Entity self, ecs::Entity other) {
    constexpr f32 selfCollisionEnableTime = 0.5f;

    auto projectileInfo = self.get<ProjectileInfo>();
    auto lifetime = self.get<Lifetime>();

    f32 timeAlive = projectileInfo.initialLifetime - lifetime.secondsRemaining;
    if (timeAlive < selfCollisionEnableTime && projectileInfo.shooterEntityID == other.id()) {
        return true;
    }
    return false;
}

static void makeExplosionParticles(Vector2i center, Vector2i surfaceNormal) {
    if (surfaceNormal.y() > 0) {
        center.e[1]--;  // so doesn't get stuck
    }
    constexpr s32 NPARTICLES = 5;
    for (s32 i = 0; i < NPARTICLES; i++) {
        const f32 maxspeed = 80;
        const f32 baselifetime = 3;
        f32 velX = System::rng.uniform() * maxspeed - maxspeed / 2;
        // upward bias
        f32 velY = System::rng.uniform() * maxspeed - maxspeed / 4;
        // f32 velY = System::rng.uniform() * maxspeed;

        if (sign(velX) == surfaceNormal.x()) {
            velX *= -1;
        }

        if (sign(velY) == surfaceNormal.y()) {
            velY *= -1;
        }

        Vector2f particleVel(velX, velY);

        // faster particles last longer
        const f32 lifetime = baselifetime * particleVel.len() / maxspeed;
        auto particle = createParticleLight(Transform2D(center), RED, lifetime).value();
        particle.add(Velocity({velX, velY}));
        particle.add(FadeOut(lifetime));

        // make sure we're not dead when this runs.
        if (lifetime > 0.1) {
            // give some time to move away from the solid we collided with before adding collider.
            particle.add(OnFrameEnd([](ecs::Entity e) {
                e.add(RigidBody({0, 0}));

                e.add(Collider::Actor(AABB(e.get<Transform2D>(), {1, 1})));
                e.get<Collider>().setMaterial(WorldMaterial::Rubber);
            }));

            f32 waitBetweenSils = lifetime / 3;
            f32 silLifetime = 0.25;
            whal::System::schedule.eventFlow({particle})
                .add(&whal::makeSilhouetteFromDraw, particle, silLifetime, Corrade::Containers::NullOpt)
                .addWait(waitBetweenSils)
                .add(&whal::makeSilhouetteFromDraw, particle, silLifetime, Corrade::Containers::NullOpt)
                .addWait(waitBetweenSils)
                .add(&whal::makeSilhouetteFromDraw, particle, silLifetime, Corrade::Containers::NullOpt);
        }
    }
}

static void makeDefaultExplosion(ecs::Entity self) {
    // lifetime's onDeath callback

    const auto projectileInfo = self.get<ProjectileInfo>();
    const Vector2f pushStrength = self.get<ProjectileInfo>().pushStrength;
    if (projectileInfo.explosionRadius > 0) {
        const Vector2i pos = self.get<Transform2D>().position;
        makeExplosionZone(pos, projectileInfo.explosionRadius, pushStrength);
        makeExplosionParticles(pos, {});
    }
}

static void Explode(ecs::Entity self, ecs::Entity other, Vector2i moveNormal) {
    auto& selfCollider = self.get<Collider>();
    if (!selfCollider.isAlive()) {
        return;
    }
    if (skipParentCollision(self, other)) {
        return;
    }

    const auto projectileInfo = self.get<ProjectileInfo>();
    const Vector2f pushStrength = projectileInfo.pushStrength;
    if (projectileInfo.explosionRadius > 0) {
        const Vector2i pos = selfCollider.getShape().getPositionEdge(moveNormal);
        makeExplosionZone(pos, projectileInfo.explosionRadius, pushStrength);
        makeExplosionParticles(pos, moveNormal);
    }
    self.kill();
    selfCollider.setIsDead();
}

Expected<ecs::Entity> makeProjectile(ecs::EntityID parentEntityID, Vector2i position, Vector2f velocity, f32 lifetimeSeconds, f32 explosionRadius,
                                     Vector2f pushStrength) {
    auto expected = System::world->entity(false);
    if (!expected.isExpected()) {
        return expected.error();
    }

    auto entity = expected.value();
    auto _ = ecs::DeferActivate(entity);

    // the sprite is pointing down by default. Get angle between for rotation
    Vector2f moveNormal = velocity.isZero() ? Vector2f::unitRight : velocity.norm();
    Vector2f referenceAngle = Vector2f::unitDown;
    f32 dot = moveNormal.dot(referenceAngle);
    f32 det = moveNormal.det(referenceAngle);
    f32 angleRadians = std::atan2(det, dot);
    Transform2D trans(position, angleRadians * RAD_TO_DEG);

    Velocity vel(velocity);
    constexpr s32 len = 6;
    constexpr s32 halflenPixels = len / 2 * PIXELS_PER_TEXEL;

    entity.add(trans);
    entity.add(vel);
    entity.add(Name("PROJECTILE"));
    entity.add(Lifetime(lifetimeSeconds, &makeDefaultExplosion));
    entity.add(PointLight({TEXELS_PER_TILE * 2, halflenPixels}));
    entity.add(ProjectileInfo{parentEntityID, lifetimeSeconds, explosionRadius, pushStrength});
    entity.add(ParticleEmitter(Colors::LightBlue, 1.0f, ParticleSetting::Light | ParticleSetting::RigidBody | ParticleSetting::Collider, 5, 15));

    static const AnimInfo animInfo = {{"effect/bluefire", 0, 4, 0.1}};
    Animator animator;
    loadAnimations(animator, animInfo);
    entity.add(animator);
    Sprite sprite = Sprite(Depth::Player, animator.getFrame());
    sprite.scale = {0.5, 0.5};
    entity.add(Draw(sprite));

    entity.add(Collider(trans, Vector2i(halflenPixels, halflenPixels), CollisionLayer::Actor, WorldMaterial::None, &Explode));

    return entity;
}

#include "Projectile.h"

#include "ECS/Callback.h"
#include "ECS/Entities/Particle.h"
#include "ECS/Light.h"
#include "ECS/RigidBody.h"
#include "Game/Components/ProjectileInfo.h"
#include "Physics/CollisionLayer.h"
#include "Physics/Material.h"
#include "Physics/Shapes.h"
#include "Settings.h"
#include "whalECS/src/ECS.h"

#include "ECS/AnimUtil.h"
#include "ECS/Animator.h"
#include "ECS/Collision.h"
#include "ECS/Draw.h"
#include "ECS/Lifetime.h"
#include "ECS/Name.h"
#include "ECS/Transform.h"
#include "ECS/Velocity.h"
#include "Explosion.h"

#include "Systems/System.h"
#include "Util/MathUtil.h"

using namespace whal;

// skip explosion if we're colliding with the entity that shot us and the shot just occurred
bool skipParentCollision(ecs::Entity self, ecs::Entity other) {
    constexpr f32 selfCollisionEnableTime = 0.5f;

    auto projectileInfo = self.get<ProjectileInfo>();
    auto lifetime = self.get<Lifetime>();

    f32 timeAlive = projectileInfo.initialLifetime - lifetime.secondsRemaining;
    if (timeAlive < selfCollisionEnableTime && projectileInfo.shooterEntityID == other.id()) {
        return true;
    }
    return false;
}

void makeExplosionParticles(Vector2i center, Vector2i surfaceNormal) {
    if (surfaceNormal.y() > 0) {
        center.e[1]--;  // so doesn't get stuck
    }
    const s32 NPARTICLES = 5;
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

        // make sure we're not dead when this runs
        if (lifetime > 0.1) {
            particle.add(OnFrameEnd([](ecs::Entity e) {
                e.add(RigidBody({0, 0}));

                auto pos = e.get<Transform2D>().position;
                e.add(Collider::Actor(AABB(pos, {1, 1})));
                e.get<Collider>().setMaterial(WorldMaterial::Rubber);
            }));
        }
    }
}

void makeDefaultExplosion(ecs::Entity self) {
    // lifetime's onDeath callback

    Vector2f pushStrength = {100, 100};
    f32 explosionRadius = self.get<Circle>().getRadius();
    if (explosionRadius > 0) {
        Vector2i pos = self.get<Transform2D>().position;
        makeExplosionZone(pos, explosionRadius, pushStrength);
        makeExplosionParticles(pos, {});
    }
}

void Explode(ecs::Entity self, ecs::Entity other, Collider* selfCollider, Collider* otherCollider, Vector2i moveNormal) {
    if (!selfCollider->isAlive()) {
        return;
    }
    if (skipParentCollision(self, other)) {
        return;
    }

    Vector2f pushStrength = {100, 100};
    f32 explosionRadius = self.get<Circle>().getRadius();
    if (explosionRadius > 0) {
        Vector2i pos = selfCollider->getShape().getPositionEdge(moveNormal);
        makeExplosionZone(pos, explosionRadius, pushStrength);
        makeExplosionParticles(pos, moveNormal);
    }
    self.kill();
    selfCollider->setIsDead();
}

void ExplodeDownwardAngle(ecs::Entity self, ecs::Entity other, Collider* selfCollider, Collider* otherCollider, Vector2i moveNormal) {
    if (!selfCollider->isAlive()) {
        return;
    }
    if (skipParentCollision(self, other)) {
        return;
    }

    Vector2f pushStrength = {100, 150};
    f32 explosionRadius = self.get<Circle>().getRadius();
    if (explosionRadius > 0) {
        Vector2i pos = selfCollider->getShape().getPositionEdge(moveNormal);
        makeExplosionZone(pos, explosionRadius, pushStrength);
        makeExplosionParticles(pos, moveNormal);
    }
    self.kill();
    selfCollider->setIsDead();
}

Expected<ecs::Entity> makeProjectile(ecs::EntityID parentEntityID, Vector2i position, Vector2f velocity, f32 lifetimeSeconds, f32 explosionRadius) {
    auto expected = System::world->entity(false);
    if (!expected.isExpected()) {
        return expected.error();
    }

    auto entity = expected.value();
    auto _ = ecs::DeferActivate(entity);

    // the sprite is pointing down by default. Get angle between for rotation
    Vector2f moveNormal = velocity.norm();
    Vector2f referenceAngle = Vector2f::unitDown;
    f32 dot = moveNormal.dot(referenceAngle);
    f32 det = moveNormal.det(referenceAngle);
    f32 angleRadians = std::atan2(det, dot);
    Transform2D trans(position, angleRadians * RAD_TO_DEG);

    Velocity vel(velocity);
    s32 len = 4;
    s32 halflenPixels = len / 2 * PIXELS_PER_TEXEL;

    entity.add(trans);
    entity.add(vel);
    entity.add(Name("PROJECTILE"));
    entity.add(Lifetime(lifetimeSeconds, &makeDefaultExplosion));
    entity.add(Circle(Vector2i(), explosionRadius));
    entity.add(PointLight({PIXELS_PER_TILE * 2, halflenPixels}));
    entity.add(ProjectileInfo{parentEntityID, lifetimeSeconds});

    static const AnimInfo animInfo = {{"effect/bluefire", 0, 4, 0.1}};
    Animator animator;
    loadAnimations(animator, animInfo);
    entity.add(animator);
    Sprite sprite = Sprite(Depth::Player, animator.getFrame());
    sprite.scale = {0.5, 0.5};
    entity.add(sprite);

    const bool isDownwardAngle = velocity.x() != 0 && velocity.y() < 0;
    entity.add(Collider(trans, Vector2i(halflenPixels, halflenPixels), CollisionLayer::Actor, WorldMaterial::None,
                        isDownwardAngle ? &ExplodeDownwardAngle : &Explode));

    return entity;
}

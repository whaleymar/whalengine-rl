#include "ParticleEmitterSystem.h"
#include "ECS/Collision.h"
#include "ECS/Entities/Particle.h"
#include "ECS/ParticleEmitter.h"
#include "ECS/RigidBody.h"
#include "ECS/Transform.h"
#include "ECS/Velocity.h"
#include "Physics/Shapes.h"
#include "Settings.h"
#include "Util/Vector.h"

namespace whal {

void ParticleEmitterSystem::update() {
    // these are between -1 and 1
    const f32 locationSampleX = (System::rng.uniform() - 0.5) * 2;
    const f32 locationSampleY = (System::rng.uniform() - 0.5) * 2;

    const Vector2f sampleSpeed = angleToUnit(360.0f * System::rng.uniform());
    const f32 spawnSample = System::rng.uniform();

    for (auto [entityid, entity] : getEntitiesRef()) {
        const auto trans = entity.get<Transform2D>();
        const auto emitter = entity.get<ParticleEmitter>();

        s32 nParticles = emitter.particlesPerSecond / 60;
        if (static_cast<f32>((emitter.particlesPerSecond % 60)) / 60.0f > spawnSample) {
            nParticles++;
        }

        if (nParticles == 0) {
            continue;
        }

        const AABB spawnZone(trans, emitter.aabbHalfTexels * PIXELS_PER_TEXEL);
        const s32 spawnOffsetX = (std::roundf((f32)spawnZone.getHalf().x() * locationSampleX));
        const s32 spawnOffsetY = (std::roundf((f32)spawnZone.getHalf().y() * locationSampleY));

        // Vector2f velocity = speedSample * emitter.maxSpeedTexelsPerSecond;
        Vector2f velocity = sampleSpeed * emitter.maxSpeedTexelsPerSecond;
        Vector2i spawnLocation;
        if ((emitter.settings & ParticleSetting::UpOnly) > 0) {
            spawnLocation = spawnZone.getPositionEdge(Vector2i::unitUp) + Vector2i(spawnOffsetX, 0);
            if (velocity.y() < 0) {
                velocity.e[1] *= -1;
            }

        } else if ((emitter.settings & ParticleSetting::LeftOnly) > 0) {
            spawnLocation = spawnZone.getPositionEdge(Vector2i::unitLeft) + Vector2i(0, spawnOffsetY);
            if (velocity.x() > 0) {
                velocity.e[0] *= -1;
            }

        } else if ((emitter.settings & ParticleSetting::RightOnly) > 0) {
            spawnLocation = spawnZone.getPositionEdge(Vector2i::unitRight) + Vector2i(0, spawnOffsetY);
            if (velocity.x() < 0) {
                velocity.e[0] *= -1;
            }

        } else if ((emitter.settings & ParticleSetting::DownOnly) > 0) {
            spawnLocation = spawnZone.getPositionEdge(Vector2i::unitDown) + Vector2i(spawnOffsetX, 0);
            if (velocity.y() > 0) {
                velocity.e[1] *= -1;
            }

        } else {
            spawnLocation = spawnZone.getPosition() + Vector2i(spawnOffsetX, spawnOffsetY);
        }

        spawnLocation += emitter.offsetTexels * PIXELS_PER_TEXEL;
        // print("spawnLocation is", spawnLocation, "with trans", trans.position, "and half", emitter.aabbHalfTexels, "and velocity", velocity);

        for (s32 i = 0; i < nParticles; i++) {
            ecs::Entity particle;
            if ((emitter.settings & ParticleSetting::Light) > 0) {
                auto eParticle = createParticleLight(Transform2D(spawnLocation), emitter.color, emitter.lifetimeSeconds, false);
                if (eParticle.isExpected()) {
                    particle = eParticle.value();
                } else {
                    continue;
                }
            } else {
                auto eParticle = createParticle(Transform2D(spawnLocation), emitter.color, emitter.lifetimeSeconds);
                if (eParticle.isExpected()) {
                    particle = eParticle.value();
                } else {
                    continue;
                }
            }

            particle.add(Velocity(velocity));

            if ((emitter.settings & ParticleSetting::RigidBody) > 0) {
                particle.add<RigidBody>();
            }

            if ((emitter.settings & ParticleSetting::Collider) > 0) {
                particle.add(Collider::Actor(AABB(spawnLocation, {1, 1})));
                particle.get<Collider>().setMaterial(WorldMaterial::Rubber);
            }
        }
    }
}

}  // namespace whal

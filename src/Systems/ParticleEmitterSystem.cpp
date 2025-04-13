#include "ParticleEmitterSystem.h"

#include "Components/ParticleEmitter.h"
#include "Components/Transform.h"
#include "Components/Velocity.h"
#include "Entities/Particle.h"
#include "Physics/MaterialData.h"
#include "Physics/Shapes.h"
#include "Settings.h"
#include "Sys/System.h"
#include "Sys/Time.h"
#include "Util/MathUtil.h"
#include "Util/Vector.h"

namespace whal {
constexpr f32 PERPENDICULAR_DAMPING = 0.33;

void ParticleEmitterSystem::update() {
    // these are between -1 and 1
    const f32 locationSampleX = (Rng.uniform() - 0.5) * 2;
    const f32 locationSampleY = (Rng.uniform() - 0.5) * 2;

    const Vector2f sampleSpeed = Vector2f::fromAngleFast(360.0f * Rng.uniform());

    for (auto [entityid, entity] : getEntities()) {
        const Transform& trans = entity.get<Transform>();
        const ParticleEmitter& emitter = entity.get<ParticleEmitter>();

        f32 nParticlesFloat = static_cast<f32>(emitter.particlesPerSecond) * Time.dt();
        s32 nParticles = nParticlesFloat;
        if (math::getDecimal(nParticlesFloat) >= Rng.uniform()) {
            nParticles++;
        }

        if (nParticles == 0) {
            continue;
        }

        const AABB spawnZone(trans, emitter.aabbHalf);
        const s32 spawnOffsetX = (std::roundf((f32)spawnZone.getHalf().x * locationSampleX));
        const s32 spawnOffsetY = (std::roundf((f32)spawnZone.getHalf().y * locationSampleY));

        Vector2f velocity = sampleSpeed * emitter.maxSpeed;
        Vector2i spawnLocation;
        if (emitter.direction == CollisionDir::UP) {
            spawnLocation = spawnZone.getPositionEdge(Vector2i::UP) + Vector2i(spawnOffsetX, 0);
            velocity.x *= PERPENDICULAR_DAMPING;
            if (velocity.y < 0) {
                velocity.y *= -1;
            }

        } else if (emitter.direction == CollisionDir::LEFT) {
            spawnLocation = spawnZone.getPositionEdge(Vector2i::LEFT) + Vector2i(0, spawnOffsetY);
            velocity.y *= PERPENDICULAR_DAMPING;
            if (velocity.x > 0) {
                velocity.x *= -1;
            }

        } else if (emitter.direction == CollisionDir::RIGHT) {
            spawnLocation = spawnZone.getPositionEdge(Vector2i::RIGHT) + Vector2i(0, spawnOffsetY);
            velocity.y *= PERPENDICULAR_DAMPING;
            if (velocity.x < 0) {
                velocity.x *= -1;
            }

        } else if (emitter.direction == CollisionDir::DOWN) {
            spawnLocation = spawnZone.getPositionEdge(Vector2i::DOWN) + Vector2i(spawnOffsetX, 0);
            velocity.x *= PERPENDICULAR_DAMPING;
            if (velocity.y > 0) {
                velocity.y *= -1;
            }

        } else {
            spawnLocation = spawnZone.getPosition() + Vector2i(spawnOffsetX, spawnOffsetY);
        }

        spawnLocation += emitter.offset;
        for (s32 i = 0; i < nParticles; i++) {
            // Create particle as child of emitter so it inherits its transform. Then orphan afterwards.
            ecs::Entity particle = entity.createChild(false);
            if (!particle.isValid()) {
                continue;
            }
            particle.orphan();
            if constexpr (WORLD_TYPE == WorldType2D::TopDown) {
                // add some random float value
                s32 floatToAdd = Rng.range(0, PIXELS_PER_TILE);
                spawnLocation.y -= floatToAdd;
                particle.set(TransformBuilder(trans)
                                 .position(spawnLocation.as<f32>())
                                 .height(trans.floatHeight + static_cast<f32>(floatToAdd))
                                 .depth(emitter.depth)
                                 .build());
            } else {
                particle.set(TransformBuilder(trans).position(spawnLocation.as<f32>()).depth(emitter.depth).build());
            }
            addParticleComponents(particle, MaterialData::get(emitter.material), emitter.lifetimeMultiplier);

            particle.add(Velocity::from(velocity));
            particle.activate();
        }
    }
}

}  // namespace whal

#include "ParticleEmitterSystem.h"

#include "Components/ParticleEmitter.h"
#include "Components/Transform.h"
#include "Components/Velocity.h"
#include "Entities/Particle.h"
#include "Physics/Shapes.h"
#include "Sys/System.h"
#include "Util/Vector.h"

namespace whal {
constexpr f32 PERPENDICULAR_DAMPING = 0.33;

void ParticleEmitterSystem::update() {
    // these are between -1 and 1
    const f32 locationSampleX = (System::rng.uniform() - 0.5) * 2;
    const f32 locationSampleY = (System::rng.uniform() - 0.5) * 2;

    const Vector2f sampleSpeed = angleToUnitFast(360.0f * System::rng.uniform());

    for (auto [entityid, entity] : getEntitiesMutable()) {
        const auto trans = entity.get<Transform2D>();
        const auto emitter = entity.get<ParticleEmitter>();

        s32 nParticles = std::round(static_cast<f32>(emitter.particlesPerSecond / 60) * System::time.getMultiplier());
        const f32 spawnSample = System::rng.uniform();
        if (static_cast<f32>((emitter.particlesPerSecond % 60)) / 60.0f * System::time.getMultiplier() > spawnSample) {
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
            spawnLocation = spawnZone.getPositionEdge(Vector2i::unitUp) + Vector2i(spawnOffsetX, 0);
            velocity.x *= PERPENDICULAR_DAMPING;
            if (velocity.y < 0) {
                velocity.y *= -1;
            }

        } else if (emitter.direction == CollisionDir::LEFT) {
            spawnLocation = spawnZone.getPositionEdge(Vector2i::unitLeft) + Vector2i(0, spawnOffsetY);
            velocity.y *= PERPENDICULAR_DAMPING;
            if (velocity.x > 0) {
                velocity.x *= -1;
            }

        } else if (emitter.direction == CollisionDir::RIGHT) {
            spawnLocation = spawnZone.getPositionEdge(Vector2i::unitRight) + Vector2i(0, spawnOffsetY);
            velocity.y *= PERPENDICULAR_DAMPING;
            if (velocity.x < 0) {
                velocity.x *= -1;
            }

        } else if (emitter.direction == CollisionDir::DOWN) {
            spawnLocation = spawnZone.getPositionEdge(Vector2i::unitDown) + Vector2i(spawnOffsetX, 0);
            velocity.x *= PERPENDICULAR_DAMPING;
            if (velocity.y > 0) {
                velocity.y *= -1;
            }

        } else {
            spawnLocation = spawnZone.getPosition() + Vector2i(spawnOffsetX, spawnOffsetY);
        }

        spawnLocation += emitter.offset;
        for (s32 i = 0; i < nParticles; i++) {
            auto eParticle = createParticle(Transform2D(spawnLocation), emitter.material, emitter.depth, emitter.lifetimeMultiplier);
            if (!eParticle.isExpected()) {
                continue;
            }
            auto particle = eParticle.value();

            particle.add(Velocity(velocity));
        }
    }
}

}  // namespace whal

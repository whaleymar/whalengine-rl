#include "ParticleEmitter.h"

#include "Map/TiledParse.h"
#include "Util/JsonUtil.h"

namespace whal {

void ParticleEmitter::loadImpl(ecs::Entity entity, void* data) {
    const LoadContext& ctx = *static_cast<LoadContext*>(data);
    ParticleEmitter emitter = entity.has<ParticleEmitter>() ? entity.get<ParticleEmitter>() : ParticleEmitter{};

    tryRead(ctx.values, "maxSpeed", &emitter.maxSpeed);
    tryRead(ctx.values, "particlesPerSecond", &emitter.particlesPerSecond);
    tryReadVal(ctx.values, "Direction", &emitter.direction);
    tryReadVal(ctx.values, "Material", &emitter.material);
    tryRead(ctx.values, "Depth", &emitter.depth);
    tryRead(ctx.values, "LifetimeMultiplier", &emitter.lifetimeMultiplier);

    Shape emitterShape = readShapeOrDefault(ctx, entity, "Shape", &emitter.offset);
    emitter.aabbHalf = emitterShape.getAABB().getHalf();
    entity.add(emitter);
}

}  // namespace whal

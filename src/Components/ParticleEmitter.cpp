#include "ParticleEmitter.h"

namespace whal {

void ParticleEmitter::setDirection(CollisionDir dir) {
    s32 flag = 0;
    switch (dir) {
    case CollisionDir::UP:
        flag = ParticleSetting::UpOnly;
        break;

    case CollisionDir::DOWN:
        flag = ParticleSetting::DownOnly;
        break;

    case CollisionDir::LEFT:
        flag = ParticleSetting::LeftOnly;
        break;

    case CollisionDir::RIGHT:
        flag = ParticleSetting::RightOnly;
        break;

    default:
        break;
    }

    settings |= flag;
}

}  // namespace whal

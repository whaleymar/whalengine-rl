#include "RigidBody.h"

namespace whal {

void RigidBody::setGrounded(WorldMaterial material) {
    isGrounded = true;
    groundMaterial = material;
}

void RigidBody::setNotGrounded() {
    isGrounded = false;
    groundMaterial = WorldMaterial::None;
}

}  // namespace whal

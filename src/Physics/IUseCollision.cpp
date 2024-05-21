#include "IUseCollision.h"
#include "Util/Print.h"

namespace whal {

// IUseCollision(AABB collider, Material material = Material::None) : mCollider(collider), mMaterial(material){};
IUseCollision::IUseCollision(AABB collider, Material material, CollisionCallback callback)
    : mCollider(collider), mOnCollisionEnter(callback), mMaterial(material) {}

void IUseCollision::setCollisionCallback(CollisionCallback callback) {
    print("this shouldn't get called");
    mOnCollisionEnter = callback;
}

bool checkDirectionalCollision(const AABB& actor, const AABB& solid, Vector2i moveNormal, CollisionDir collisionDir) {
    switch (collisionDir) {
    case CollisionDir::ALL:
        return true;

    case CollisionDir::LEFT:
        if (moveNormal.x() <= 0 || actor.right() != solid.left()) {
            return false;
        }
        break;

    case CollisionDir::RIGHT:
        if (moveNormal.x() >= 0 || actor.left() != solid.right()) {
            return false;
        }
        break;

    case CollisionDir::DOWN:
        if (moveNormal.y() <= 0 || actor.top() != solid.bottom()) {
            return false;
        }
        break;

    case CollisionDir::UP:
        if (moveNormal.y() >= 0 || actor.bottom() != solid.top()) {
            return false;
        }
        break;
    }

    return true;
}

}  // namespace whal

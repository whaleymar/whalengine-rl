#include "IUseCollision.h"
#include "Util/Print.h"

namespace whal {

// IUseCollision(AABB collider, Material material = Material::None) : mCollider(collider), mMaterial(material){};
IUseCollision::IUseCollision(AABB collider, WorldMaterial material, CollisionCallback callback)
    : mCollider(collider), mOnCollisionEnter(callback), mMaterial(material) {}

void IUseCollision::setCollisionCallback(CollisionCallback callback) {
    print("this shouldn't get called");
    mOnCollisionEnter = callback;
}

bool checkDirectionalCollision(const AABB& movingCollider, const AABB& oneWayCollider, Vector2i movement, CollisionDir collisionDir) {
    // check if other's movement will collide with the one way collider
    switch (collisionDir) {
    case CollisionDir::ALL:
        return true;

    case CollisionDir::LEFT:
        if (movement.x() <= 0 || movingCollider.right() != oneWayCollider.left()) {
            return false;
        }
        break;

    case CollisionDir::RIGHT:
        if (movement.x() >= 0 || movingCollider.left() != oneWayCollider.right()) {
            return false;
        }
        break;

    case CollisionDir::DOWN:
        if (movement.y() <= 0 || movingCollider.top() != oneWayCollider.bottom()) {
            return false;
        }
        break;

    case CollisionDir::UP:
        if (movement.y() >= 0 || movingCollider.bottom() != oneWayCollider.top()) {
            return false;
        }
        break;
    }

    return true;
}

}  // namespace whal

#include "IUseCollision.h"
#include "Util/Print.h"

namespace whal {

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

    case CollisionDir::LEFT: {
        bool isSweep =
            movement.x() > 1 && movingCollider.right() <= oneWayCollider.left() && (movingCollider.right() + movement.x()) >= oneWayCollider.left();
        if (movement.x() <= 0 || ((movement.x() >= 1 && movingCollider.right() != oneWayCollider.left()) && !isSweep)) {
            return false;
        }
        break;
    }

    case CollisionDir::RIGHT: {
        bool isSweep =
            movement.x() < -1 && movingCollider.left() >= oneWayCollider.right() && (movingCollider.left() + movement.x()) <= oneWayCollider.right();
        if (movement.x() >= 0 || ((movement.x() <= -1 && movingCollider.left() != oneWayCollider.right()) && !isSweep)) {
            return false;
        }
        break;
    }

    case CollisionDir::DOWN: {
        bool isSweep = movement.y() > 1 && movingCollider.top() <= oneWayCollider.bottom() &&
                       (movingCollider.bottom() + movement.y()) >= oneWayCollider.bottom();
        if (movement.y() <= 0 || ((movement.y() >= 1 && movingCollider.top() != oneWayCollider.bottom()) && !isSweep)) {
            return false;
        }
        break;
    }

    case CollisionDir::UP: {
        bool isSweep =
            movement.y() < -1 && movingCollider.bottom() >= oneWayCollider.top() && (movingCollider.bottom() + movement.y()) <= oneWayCollider.top();
        // if (true) {
        //     print("movement: ", movement, "\nactor bottom: ", movingCollider.bottom(), "\nSolid top: ", oneWayCollider.top(), "\n\n");
        // }
        if (movement.y() >= 0 || ((movement.y() <= -1 && movingCollider.bottom() != oneWayCollider.top()) && !isSweep)) {
            return false;
        }
        break;
    }
    }

    return true;
}

}  // namespace whal

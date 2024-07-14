#include "Quadtree.h"

#include <unordered_map>

namespace whal::qtree {

static std::unordered_map<ecs::Entity, AABB, ecs::EntityHash> S_COLLIDER_SHAPES;

AABB getShape(const ecs::Entity value) {
    return S_COLLIDER_SHAPES.at(value);
}

void setShape(const ecs::Entity value, const AABB shape) {
    S_COLLIDER_SHAPES[value] = shape;
}

void removeShape(const ecs::Entity value) {
    S_COLLIDER_SHAPES.erase(value);
}

}  // namespace whal::qtree

#pragma once

#include <array>
#include <cassert>
#include <memory>
#include <vector>

#include "Physics/CollisionLayer.h"
#include "Physics/Shapes.h"
#include "whalECS/src/ECS.h"

namespace whal {

struct Segment;
struct RaycastHit;

namespace qtree {

// TODO this is quite slow for moving entities, since they require multiple removals/adds per frame
// but it's very fast for things that don't move.
// should use this for entities without a velocity component, but a spatial grid for things with velocity,
// and then a lookup would basically dispatch the correct data struct based on entity.has<Velocity>()
class QuadTree {
    struct Value {
        ecs::Entity entity;
        AABB shape;
    };

public:
    QuadTree(const AABB& boundingBox);

    void add(const ecs::Entity value);
    void add(const ecs::Entity value, const AABB& newShape);

    void remove(const ecs::Entity value);
    void remove(const ecs::Entity value, const AABB& previousShape);

    std::vector<ecs::Entity> query(const AABB& box) const;
    std::vector<std::pair<ecs::Entity, ecs::Entity>> findAllIntersections() const;

    AABB getBoundingBox() const;

    // Note: does not detect entities whose collider contains the ray's origin point
    // Note: direction does not have to be normalized
    RaycastHit raycast(Vector2f origin, Vector2f direction, f32 maxDistance, u16 layerMask = CollisionLayer::ALL);

    RaycastHit circlecast(Vector2f origin, Vector2f direction, f32 maxDistance, f32 radius, u16 layerMask = CollisionLayer::ALL);

private:
    struct Node {
        std::array<std::unique_ptr<Node>, 4> children;
        std::vector<Value> values;
    };

    AABB mBoundingBox;
    std::unique_ptr<Node> mRoot;

    bool isLeaf(const Node* node) const;
    AABB computeBox(const AABB& box, int i) const;

    // returns quadrant index
    // 0 1
    // 2 3
    s32 getQuadrant(const AABB& nodeBox, const AABB& valueBox) const;
    void add(Node* node, s32 depth, const AABB& parentBox, const Value value);
    void split(Node* node, const AABB& parentBox);
    bool remove(Node* node, const AABB& parentBox, const Value value);
    void removeValue(Node* node, const Value value);
    bool tryMerge(Node* node);
    void query(Node* node, const AABB& box, const AABB& queryBox, std::vector<ecs::Entity>& values) const;
    void querySegment(Node* node, const AABB& box, const Segment& querySegment, std::vector<ecs::Entity>& values) const;
    void findAllIntersections(Node* node, std::vector<std::pair<ecs::Entity, ecs::Entity>>& intersections) const;
    void findIntersectionsInDescendants(Node* node, const Value value, std::vector<std::pair<ecs::Entity, ecs::Entity>>& intersections) const;
    RaycastHit _raycast(Segment ray, u16 layerMask);
};

}  // namespace qtree
}  // namespace whal

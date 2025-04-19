#pragma once

#include <cassert>
#include <vector>

#include "Physics/CollisionLayer.h"
#include "Physics/Shapes.h"
#include "whalECS/src/ECS.h"

namespace whal {

struct Segment;
struct RaycastHit;

namespace qtree {

class QuadTree {
    struct Value {
        ecs::Entity entity;
        AABB shape;
    };

    struct Node {
        // points to the first child, or -1 if none
        s32 firstChildIx = -1;
        std::vector<Value> values;

        bool isLeaf() const { return firstChildIx == -1; }

        void reset() {
            firstChildIx = -1;
            values.clear();
        }
    };

public:
    QuadTree(const AABB& boundingBox);

    void add(const ecs::Entity value);
    void add(const ecs::Entity value, const AABB& newShape);

    void remove(const ecs::Entity value);
    void remove(const ecs::Entity value, const AABB& previousShape);

    std::vector<ecs::Entity> query(const AABB& box) const;
    std::vector<std::pair<ecs::Entity, ecs::Entity>> findAllIntersections() const;

    RaycastHit raycast(Vector2f origin, Vector2f direction, f32 maxDistance, u16 layerMask = CollisionLayer::ALL) const;
    RaycastHit circlecast(Vector2f origin, Vector2f direction, f32 maxDistance, f32 radius, u16 layerMask = CollisionLayer::ALL) const;

    AABB getBoundingBox() const;

private:
    // returns the index of a free node.
    // creates a new one if all are used
    s32 allocateNode();
    void freeNode(s32 ix);
    AABB computeBox(const AABB& box, s32 i) const;

    // returns quadrant index
    // 0 1
    // 2 3
    s32 getQuadrant(const AABB& nodeBox, const AABB& valueBox) const;
    void _add(s32 nodeIx, s32 depth, const AABB& parentBox, const Value value);
    void split(const s32 nodeIx, const AABB& parentBox);
    bool _remove(const s32 nodeIx, const AABB& parentBox, const Value value);
    void removeValue(const s32 nodeIx, const Value value);
    bool tryMerge(const s32 nodeIx);
    void _query(const s32 nodeIx, const AABB& box, const AABB& queryBox, std::vector<ecs::Entity>& values) const;
    void _querySegment(const s32 nodeIx, const AABB& box, const Segment& querySegment, std::vector<ecs::Entity>& values) const;
    void _findAllIntersections(const s32 nodeIx, std::vector<std::pair<ecs::Entity, ecs::Entity>>& intersections) const;
    void _findIntersectionsInDescendants(const s32 nodeIx, const Value value, std::vector<std::pair<ecs::Entity, ecs::Entity>>& intersections) const;
    RaycastHit _raycast(Segment ray, u16 layerMask) const;

    std::vector<Node> mNodes;       // dense node storage
    std::vector<s32> mFreeIndices;  // stack of free node indices
    AABB mBoundingBox;
    s32 mRootIx = -1;
};

}  // namespace qtree
}  // namespace whal

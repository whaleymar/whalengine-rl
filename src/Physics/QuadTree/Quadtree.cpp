#include "Quadtree.h"

#include <algorithm>

#include "Components/Collision.h"
#include "Physics/HitInfo.h"
#include "Physics/Segment.h"
#include "Util/Vector.h"

namespace whal::qtree {

static constexpr s32 THRESHOLD = 16;
static constexpr s32 MAX_DEPTH = 8;

QuadTree::QuadTree(const AABB& boundingBox) : mBoundingBox(boundingBox), mRoot(std::make_unique<Node>()) {}

void QuadTree::add(const ecs::Entity value) {
    add(mRoot.get(), 0, mBoundingBox, {value, value.get<Collider>().getShape()});
}
void QuadTree::add(const ecs::Entity value, const AABB& newShape) {
    add(mRoot.get(), 0, mBoundingBox, {value, newShape});
}

void QuadTree::remove(const ecs::Entity value) {
    remove(mRoot.get(), mBoundingBox, {value, value.get<Collider>().getShape()});
}
void QuadTree::remove(const ecs::Entity value, const AABB& previousShape) {
    remove(mRoot.get(), mBoundingBox, {value, previousShape});
}

std::vector<ecs::Entity> QuadTree::query(const AABB& box) const {
    auto values = std::vector<ecs::Entity>();
    query(mRoot.get(), mBoundingBox, box, values);
    return values;
}

std::vector<std::pair<ecs::Entity, ecs::Entity>> QuadTree::findAllIntersections() const {
    auto intersections = std::vector<std::pair<ecs::Entity, ecs::Entity>>();
    findAllIntersections(mRoot.get(), intersections);
    return intersections;
}

AABB QuadTree::getBoundingBox() const {
    return mBoundingBox;
}

RaycastHit QuadTree::raycast(Vector2f origin, Vector2f direction, f32 maxDistance, u16 layerMask) {
    direction = direction.norm();
    Segment ray(origin, direction * maxDistance);
    return _raycast(ray, layerMask);
}

RaycastHit QuadTree::circlecast(Vector2f origin, Vector2f direction, f32 maxDistance, f32 radius, u16 layerMask) {
    direction = direction.norm();
    Segment ray(origin, direction * maxDistance, radius);
    return _raycast(ray, layerMask);
}

RaycastHit QuadTree::_raycast(Segment ray, u16 layerMask) {
    auto values = std::vector<ecs::Entity>();
    querySegment(mRoot.get(), mBoundingBox, ray, values);

    RaycastHit hitinfo;
    if (values.size() == 0) {
        return hitinfo;
    }

    // Get closest entity whose collider doesn't contain the origin point
    const Vector2i originI = ray.origin.as<s32>();
    f32 closestDistance = 1e10;
    // research should i get a vector of Value structs instead so I already have the colliders?
    for (auto entity : values) {
        const auto collider = entity.get<Collider>();
        const auto shape = collider.getShape();
        if ((layerMask & collider.getCollisionLayer()) == 0 || shape.contains(originI)) {
            continue;
        }

        const RaycastHit curHitInfo = ray.collide(shape);
        if (curHitInfo.distance < closestDistance) {
            closestDistance = curHitInfo.distance;

            // Properly calculate float distance
            hitinfo = curHitInfo;
            hitinfo.setOther(entity);
            hitinfo.otherLayer = collider.getCollisionLayer();
            hitinfo.otherMaterial = collider.getMaterial();
        }
    }

    return hitinfo;
}

bool QuadTree::isLeaf(const Node* node) const {
    return !static_cast<bool>(node->children[0]);
}

AABB QuadTree::computeBox(const AABB& box, int i) const {
    assert(i >= 0 && i <= 3 && "i not between 0-3");

    auto center = box.getPosition();
    const Vector2f exactHalf = box.getHalf().as<f32>() / 2;
    const Vector2i biggerHalf(std::round(exactHalf.x), std::round(exactHalf.y));

    // if the current quadrant's half is an odd number on either axis, give the extra pixel to the West/South halves.
    const Vector2i smallerHalf = box.getHalf() - biggerHalf;

    switch (i) {
    // North West
    case 0: {
        Vector2i halflen(biggerHalf.x, smallerHalf.y);
        return AABB(center + Vector2i(-halflen.x, halflen.y), halflen);
    }
    // North East
    case 1: {
        Vector2i halflen(smallerHalf.x, smallerHalf.y);
        return AABB(center + Vector2i(halflen.x, halflen.y), halflen);
    }
    // South West
    case 2: {
        Vector2i halflen(biggerHalf.x, biggerHalf.y);
        return AABB(center + Vector2i(-halflen.x, -halflen.y), halflen);
    }
    // South East
    case 3: {
        Vector2i halflen(smallerHalf.x, biggerHalf.y);
        return AABB(center + Vector2i(halflen.x, -halflen.y), halflen);
    }
    default:
        return AABB();  // should never run due to assert
    }
}

// returns quadrant index
// 0 1
// 2 3
s32 QuadTree::getQuadrant(const AABB& nodeBox, const AABB& valueBox) const {
    auto center = nodeBox.getPosition();
    // West
    if (valueBox.right() < center.x) {
        // South West
        if (valueBox.top() < center.y) {
            return 2;
        }

        // North West
        else if (valueBox.bottom() >= center.y) {
            return 0;
        }

        // Not contained in any quadrant
        else {
            return -1;
        }
    }
    // East
    else if (valueBox.left() >= center.x) {
        // South East
        if (valueBox.top() < center.y) {
            return 3;
        }

        // North East
        else if (valueBox.bottom() >= center.y) {
            return 1;
        }

        // Not contained in any quadrant
        else {
            return -1;
        }
    }
    // Not contained in any quadrant
    else {
        return -1;
    }
}

void QuadTree::add(Node* node, s32 depth, const AABB& parentBox, const Value value) {
    assert(node != nullptr);
    assert(parentBox.contains(value.shape));
    if (isLeaf(node)) {
        // Insert the value in this node if possible
        if (depth >= MAX_DEPTH || node->values.size() < THRESHOLD) {
            // print("hit max depth");
            node->values.push_back(value);
        }
        // Otherwise, we split and we try again
        else {
            split(node, parentBox);
            add(node, depth, parentBox, value);
        }
    } else {
        auto i = getQuadrant(parentBox, value.shape);
        // Add the value in a child if the value is entirely contained in it
        if (i != -1) {
            add(node->children[static_cast<std::size_t>(i)].get(), depth + 1, computeBox(parentBox, i), value);
        }
        // Otherwise, we add the value in the current node
        else {
            node->values.push_back(value);
        }
    }
}

void QuadTree::split(Node* node, const AABB& parentBox) {
    assert(node != nullptr);
    assert(isLeaf(node) && "Only leaves can be split");
    // Create children
    for (auto& child : node->children)
        child = std::make_unique<Node>();
    // Assign values to children
    auto newValues = std::vector<Value>();  // New values for this node
    for (const auto& value : node->values) {
        auto i = getQuadrant(parentBox, value.shape);
        if (i != -1)
            node->children[static_cast<std::size_t>(i)]->values.push_back(value);
        else
            newValues.push_back(value);
    }
    node->values = std::move(newValues);
}

bool QuadTree::remove(Node* node, const AABB& parentBox, const Value value) {
    assert(node != nullptr);
    assert(parentBox.contains(value.shape));
    if (isLeaf(node)) {
        // Remove the value from node
        removeValue(node, value);
        return true;
    } else {
        // Remove the value in a child if the value is entirely contained in it
        auto i = getQuadrant(parentBox, value.shape);
        if (i != -1) {
            if (remove(node->children[static_cast<std::size_t>(i)].get(), computeBox(parentBox, i), value))
                return tryMerge(node);
        }
        // Otherwise, we remove the value from the current node
        else {
            removeValue(node, value);
        }

        return false;
    }
}

void QuadTree::removeValue(Node* node, const Value value) {
    // Find the value in node->values
    auto it = std::find_if(std::begin(node->values), std::end(node->values), [value](const Value other) { return value.entity == other.entity; });
    assert(it != std::end(node->values) && "Trying to remove a value that is not present in the node");
    // Swap with the last element and pop back
    *it = std::move(node->values.back());
    node->values.pop_back();
}

bool QuadTree::tryMerge(Node* node) {
    assert(node != nullptr);
    assert(!isLeaf(node) && "Only interior nodes can be merged");
    auto nbValues = node->values.size();
    for (const auto& child : node->children) {
        if (!isLeaf(child.get()))
            return false;
        nbValues += child->values.size();
    }
    if (nbValues <= THRESHOLD) {
        node->values.reserve(nbValues);
        // Merge the values of all the children
        for (const auto& child : node->children) {
            for (const auto& value : child->values)
                node->values.push_back(value);
        }
        // Remove the children
        for (auto& child : node->children)
            child.reset();
        return true;
    } else
        return false;
}

void QuadTree::query(Node* node, const AABB& box, const AABB& queryBox, std::vector<ecs::Entity>& values) const {
    // assert(node != nullptr);
    // assert(queryBox.isOverlapping(box));
    for (const auto& value : node->values) {
        if (queryBox.isOverlapping(value.shape))
            values.push_back(value.entity);
    }
    if (!isLeaf(node)) {
        for (auto i = std::size_t(0); i < node->children.size(); ++i) {
            auto childBox = computeBox(box, static_cast<int>(i));
            if (queryBox.isOverlapping(childBox))
                query(node->children[i].get(), childBox, queryBox, values);
        }
    }
}

void QuadTree::querySegment(Node* node, const AABB& box, const Segment& segment, std::vector<ecs::Entity>& values) const {
    // assert(node != nullptr);
    // assert(queryBox.isOverlapping(box));
    for (const auto& value : node->values) {
        if (segment.isIntersecting(value.shape))
            values.push_back(value.entity);
    }
    if (!isLeaf(node)) {
        for (auto i = std::size_t(0); i < node->children.size(); ++i) {
            auto childBox = computeBox(box, static_cast<int>(i));
            if (segment.isIntersecting(childBox))
                querySegment(node->children[i].get(), childBox, segment, values);
        }
    }
}

void QuadTree::findAllIntersections(Node* node, std::vector<std::pair<ecs::Entity, ecs::Entity>>& intersections) const {
    // Find intersections between values stored in this node
    // Make sure to not report the same intersection twice
    for (auto i = std::size_t(0); i < node->values.size(); ++i) {
        for (auto j = std::size_t(0); j < i; ++j) {
            if (node->values[i].shape.isOverlapping(node->values[j].shape))
                intersections.emplace_back(node->values[i].entity, node->values[j].entity);
        }
    }
    if (!isLeaf(node)) {
        // Values in this node can intersect values in descendants
        for (const auto& child : node->children) {
            for (const auto& value : node->values)
                findIntersectionsInDescendants(child.get(), value, intersections);
        }
        // Find intersections in children
        for (const auto& child : node->children)
            findAllIntersections(child.get(), intersections);
    }
}

void QuadTree::findIntersectionsInDescendants(Node* node, const Value value, std::vector<std::pair<ecs::Entity, ecs::Entity>>& intersections) const {
    // Test against the values stored in this node
    for (const auto& other : node->values) {
        if (value.shape.isOverlapping(other.shape))
            intersections.emplace_back(value.entity, other.entity);
    }
    // Test against values stored into descendants of this node
    if (!isLeaf(node)) {
        for (const auto& child : node->children)
            findIntersectionsInDescendants(child.get(), value, intersections);
    }
}

}  // namespace whal::qtree

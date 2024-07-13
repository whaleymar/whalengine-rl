#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <memory>
#include <vector>

#include "Components/Collision.h"
#include "Physics/Shapes.h"
#include "Util/Vector.h"
#include "whalECS/src/ECS.h"

namespace whal::qtree {

// TODO this is quite slow for moving entities, since they require multiple removals/adds per frame
// but it's very fast for things that don't move.
// should use this for entities without a velocity component, but a spatial grid for things with velocity,
// and then a lookup would basically dispatch the correct data struct based on entity.has<Velocity>()
class QuadTree {
public:
    QuadTree(const AABB& boundingBox) : mBoundingBox(boundingBox), mRoot(std::make_unique<Node>()) {}

    void add(const ecs::Entity value) { add(mRoot.get(), 0, mBoundingBox, value); }

    void remove(const ecs::Entity value) { remove(mRoot.get(), mBoundingBox, value); }

    std::vector<ecs::Entity> query(const AABB& box) const {
        auto values = std::vector<ecs::Entity>();
        query(mRoot.get(), mBoundingBox, box, values);
        return values;
    }

    std::vector<std::pair<ecs::Entity, ecs::Entity>> findAllIntersections() const {
        auto intersections = std::vector<std::pair<ecs::Entity, ecs::Entity>>();
        findAllIntersections(mRoot.get(), intersections);
        return intersections;
    }

    AABB getBoundingBox() const { return mBoundingBox; }

private:
    static constexpr s32 THRESHOLD = 16;
    static constexpr s32 MAX_DEPTH = 8;

    struct Node {
        std::array<std::unique_ptr<Node>, 4> children;
        std::vector<ecs::Entity> values;
    };

    AABB mBoundingBox;
    std::unique_ptr<Node> mRoot;

    bool isLeaf(const Node* node) const { return !static_cast<bool>(node->children[0]); }

    AABB computeBox(const AABB& box, int i) const {
        assert(i >= 0 && i <= 3 && "i not between 0-3");

        auto center = box.getPosition();
        const Vector2f exactHalf = toFloatVec(box.getHalf()) / 2;
        const Vector2i biggerHalf(std::round(exactHalf.x()), std::round(exactHalf.y()));

        // if the current quadrant's half is an odd number on either axis, give the extra pixel to the West/South halves.
        const Vector2i smallerHalf = box.getHalf() - biggerHalf;

        switch (i) {
        // North West
        case 0: {
            Vector2i halflen(biggerHalf.x(), smallerHalf.y());
            return AABB(center + Vector2i(-halflen.x(), halflen.y()), halflen);
        }
        // North East
        case 1: {
            Vector2i halflen(smallerHalf.x(), smallerHalf.y());
            return AABB(center + Vector2i(halflen.x(), halflen.y()), halflen);
        }
        // South West
        case 2: {
            Vector2i halflen(biggerHalf.x(), biggerHalf.y());
            return AABB(center + Vector2i(-halflen.x(), -halflen.y()), halflen);
        }
        // South East
        case 3: {
            Vector2i halflen(smallerHalf.x(), biggerHalf.y());
            return AABB(center + Vector2i(halflen.x(), -halflen.y()), halflen);
        }
        default:
            return AABB();  // should never run due to assert
        }
    }

    s32 getQuadrant(const AABB& nodeBox, const AABB& valueBox) const {
        auto center = nodeBox.getPosition();
        // West
        if (valueBox.right() < center.x()) {
            // South West
            if (valueBox.top() < center.y())
                return 2;
            // North West
            else if (valueBox.bottom() >= center.y())
                return 0;
            // Not contained in any quadrant
            else
                return -1;
        }
        // East
        else if (valueBox.left() >= center.x()) {
            // South East
            if (valueBox.top() < center.y())
                return 3;
            // North East
            else if (valueBox.bottom() >= center.y())
                return 1;
            // Not contained in any quadrant
            else
                return -1;
        }
        // Not contained in any quadrant
        else
            return -1;
    }

    void add(Node* node, s32 depth, const AABB& parentBox, const ecs::Entity value) {
        assert(node != nullptr);
        assert(parentBox.contains(value.get<Collider>().getShape()));
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
            auto i = getQuadrant(parentBox, value.get<Collider>().getShape());
            // Add the value in a child if the value is entirely contained in it
            if (i != -1)
                add(node->children[static_cast<std::size_t>(i)].get(), depth + 1, computeBox(parentBox, i), value);
            // Otherwise, we add the value in the current node
            else
                node->values.push_back(value);
        }
    }

    void split(Node* node, const AABB& parentBox) {
        assert(node != nullptr);
        assert(isLeaf(node) && "Only leaves can be split");
        // Create children
        for (auto& child : node->children)
            child = std::make_unique<Node>();
        // Assign values to children
        auto newValues = std::vector<ecs::Entity>();  // New values for this node
        for (const auto& value : node->values) {
            auto i = getQuadrant(parentBox, value.get<Collider>().getShape());
            if (i != -1)
                node->children[static_cast<std::size_t>(i)]->values.push_back(value);
            else
                newValues.push_back(value);
        }
        node->values = std::move(newValues);
    }

    bool remove(Node* node, const AABB& parentBox, const ecs::Entity value) {
        assert(node != nullptr);
        assert(parentBox.contains(value.get<Collider>().getShape()));
        if (isLeaf(node)) {
            // Remove the value from node
            removeValue(node, value);
            return true;
        } else {
            // Remove the value in a child if the value is entirely contained in it
            auto i = getQuadrant(parentBox, value.get<Collider>().getShape());
            if (i != -1) {
                if (remove(node->children[static_cast<std::size_t>(i)].get(), computeBox(parentBox, i), value))
                    return tryMerge(node);
            }
            // Otherwise, we remove the value from the current node
            else
                removeValue(node, value);
            return false;
        }
    }

    void removeValue(Node* node, const ecs::Entity value) {
        // Find the value in node->values
        auto it = std::find_if(std::begin(node->values), std::end(node->values), [value](const ecs::Entity other) { return value == other; });
        assert(it != std::end(node->values) && "Trying to remove a value that is not present in the node");
        // Swap with the last element and pop back
        *it = std::move(node->values.back());
        node->values.pop_back();
    }

    bool tryMerge(Node* node) {
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

    void query(Node* node, const AABB& box, const AABB& queryBox, std::vector<ecs::Entity>& values) const {
        assert(node != nullptr);
        assert(queryBox.isOverlapping(box));
        for (const auto& value : node->values) {
            if (queryBox.isOverlapping(value.get<Collider>().getShape()))
                values.push_back(value);
        }
        if (!isLeaf(node)) {
            for (auto i = std::size_t(0); i < node->children.size(); ++i) {
                auto childBox = computeBox(box, static_cast<int>(i));
                if (queryBox.isOverlapping(childBox))
                    query(node->children[i].get(), childBox, queryBox, values);
            }
        }
    }

    void findAllIntersections(Node* node, std::vector<std::pair<ecs::Entity, ecs::Entity>>& intersections) const {
        // Find intersections between values stored in this node
        // Make sure to not report the same intersection twice
        for (auto i = std::size_t(0); i < node->values.size(); ++i) {
            for (auto j = std::size_t(0); j < i; ++j) {
                if (node->values[i].get<Collider>().getShape().isOverlapping(node->values[j].get<Collider>().getShape()))
                    intersections.emplace_back(node->values[i], node->values[j]);
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

    void findIntersectionsInDescendants(Node* node, const ecs::Entity value, std::vector<std::pair<ecs::Entity, ecs::Entity>>& intersections) const {
        // Test against the values stored in this node
        for (const auto& other : node->values) {
            if (value.get<Collider>().getShape().isOverlapping(other.get<Collider>().getShape()))
                intersections.emplace_back(value, other);
        }
        // Test against values stored into descendants of this node
        if (!isLeaf(node)) {
            for (const auto& child : node->children)
                findIntersectionsInDescendants(child.get(), value, intersections);
        }
    }
};

}  // namespace whal::qtree

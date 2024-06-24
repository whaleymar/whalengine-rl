#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <memory>
#include <vector>

#include "Physics/Shapes.h"
#include "Util/Vector.h"

namespace whal::qtree {

template <typename T, typename GetBox, typename Equal = std::equal_to<T>, typename Float = float>
class Quadtree {
    static_assert(std::is_convertible_v<std::invoke_result_t<GetBox, const T&>, AABB>, "GetBox must be a callable of signature AABB(const T&)");
    static_assert(std::is_convertible_v<std::invoke_result_t<Equal, const T&, const T&>, bool>,
                  "Equal must be a callable of signature bool(const T&, const T&)");
    static_assert(std::is_arithmetic_v<Float>);

public:
    Quadtree(const AABB& box, const GetBox& getBox = GetBox(), const Equal& equal = Equal())
        : mBox(box), mRoot(std::make_unique<Node>()), mGetBox(getBox), mEqual(equal) {}

    void add(const T& value) { add(mRoot.get(), 0, mBox, value); }

    void remove(const T& value) { remove(mRoot.get(), mBox, value); }

    std::vector<T> query(const AABB& box) const {
        auto values = std::vector<T>();
        query(mRoot.get(), mBox, box, values);
        return values;
    }

    std::vector<std::pair<T, T>> findAllIntersections() const {
        auto intersections = std::vector<std::pair<T, T>>();
        findAllIntersections(mRoot.get(), intersections);
        return intersections;
    }

    AABB getBox() const { return mBox; }

private:
    static constexpr s32 THRESHOLD = 16;
    static constexpr s32 MAX_DEPTH = 8;

    struct Node {
        std::array<std::unique_ptr<Node>, 4> children;
        std::vector<T> values;
    };

    AABB mBox;
    std::unique_ptr<Node> mRoot;
    GetBox mGetBox;
    Equal mEqual;

    bool isLeaf(const Node* node) const { return !static_cast<bool>(node->children[0]); }

    AABB computeBox(const AABB& box, int i) const {
        assert(i >= 0 && i <= 3 && "i not between 0-3");

        auto center = box.getPosition();
        auto childHalfLen = box.getHalf() * 0.5;
        switch (i) {
        // North West
        case 0:
            return AABB(center + Vector2i(-childHalfLen.x(), childHalfLen.y()), childHalfLen);
        // Norst East
        case 1:
            return AABB(center + Vector2i(childHalfLen.x(), childHalfLen.y()), childHalfLen);
        // South West
        case 2:
            return AABB(center + Vector2i(-childHalfLen.x(), -childHalfLen.y()), childHalfLen);
        // South East
        case 3:
            return AABB(center + Vector2i(childHalfLen.x(), -childHalfLen.y()), childHalfLen);
        }
    }

    s32 getQuadrant(const AABB& nodeBox, const AABB& valueBox) const {
        auto center = nodeBox.getPosition();
        // West
        if (valueBox.right() < center.x()) {
            // North West
            if (valueBox.bottom() < center.y())
                return 0;
            // South West
            else if (valueBox.top() >= center.y())
                return 2;
            // Not contained in any quadrant
            else
                return -1;
        }
        // East
        else if (valueBox.left() >= center.x()) {
            // North East
            if (valueBox.bottom() < center.y())
                return 1;
            // South East
            else if (valueBox.top() >= center.y())
                return 3;
            // Not contained in any quadrant
            else
                return -1;
        }
        // Not contained in any quadrant
        else
            return -1;
    }

    void add(Node* node, s32 depth, const AABB& box, const T& value) {
        assert(node != nullptr);
        assert(box.contains(mGetBox(value)));
        if (isLeaf(node)) {
            // Insert the value in this node if possible
            if (depth >= MAX_DEPTH || node->values.size() < THRESHOLD)
                node->values.push_back(value);
            // Otherwise, we split and we try again
            else {
                split(node, box);
                add(node, depth, box, value);
            }
        } else {
            auto i = getQuadrant(box, mGetBox(value));
            // Add the value in a child if the value is entirely contained in it
            if (i != -1)
                add(node->children[static_cast<std::size_t>(i)].get(), depth + 1, computeBox(box, i), value);
            // Otherwise, we add the value in the current node
            else
                node->values.push_back(value);
        }
    }

    void split(Node* node, const AABB& box) {
        assert(node != nullptr);
        assert(isLeaf(node) && "Only leaves can be split");
        // Create children
        for (auto& child : node->children)
            child = std::make_unique<Node>();
        // Assign values to children
        auto newValues = std::vector<T>();  // New values for this node
        for (const auto& value : node->values) {
            auto i = getQuadrant(box, mGetBox(value));
            if (i != -1)
                node->children[static_cast<std::size_t>(i)]->values.push_back(value);
            else
                newValues.push_back(value);
        }
        node->values = std::move(newValues);
    }

    bool remove(Node* node, const AABB& box, const T& value) {
        assert(node != nullptr);
        assert(box.contains(mGetBox(value)));
        if (isLeaf(node)) {
            // Remove the value from node
            removeValue(node, value);
            return true;
        } else {
            // Remove the value in a child if the value is entirely contained in it
            auto i = getQuadrant(box, mGetBox(value));
            if (i != -1) {
                if (remove(node->children[static_cast<std::size_t>(i)].get(), computeBox(box, i), value))
                    return tryMerge(node);
            }
            // Otherwise, we remove the value from the current node
            else
                removeValue(node, value);
            return false;
        }
    }

    void removeValue(Node* node, const T& value) {
        // Find the value in node->values
        auto it = std::find_if(std::begin(node->values), std::end(node->values), [this, &value](const auto& rhs) { return mEqual(value, rhs); });
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

    void query(Node* node, const AABB& box, const AABB& queryBox, std::vector<T>& values) const {
        assert(node != nullptr);
        assert(queryBox.isOverlapping(box));
        for (const auto& value : node->values) {
            if (queryBox.isOverlapping(mGetBox(value)))
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

    void findAllIntersections(Node* node, std::vector<std::pair<T, T>>& intersections) const {
        // Find intersections between values stored in this node
        // Make sure to not report the same intersection twice
        for (auto i = std::size_t(0); i < node->values.size(); ++i) {
            for (auto j = std::size_t(0); j < i; ++j) {
                if (mGetBox(node->values[i]).intersects(mGetBox(node->values[j])))
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

    void findIntersectionsInDescendants(Node* node, const T& value, std::vector<std::pair<T, T>>& intersections) const {
        // Test against the values stored in this node
        for (const auto& other : node->values) {
            if (mGetBox(value).intersects(mGetBox(other)))
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

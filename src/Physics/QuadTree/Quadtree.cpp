#include "Quadtree.h"

#include "Components/Collider.h"
#include "Physics/HitInfo.h"
#include "Physics/Segment.h"
#include "Util/Vector.h"

namespace whal::qtree {

static constexpr s32 THRESHOLD = 16;  // number of entities in a node before we try splitting
static constexpr s32 MAX_DEPTH = 8;

QuadTree::QuadTree(const AABB& boundingBox) : mBoundingBox(boundingBox), mRootIx(allocateNode()) {
    // pre-allocate node lists so we don't need to re-allocate during level loads
    mNodes.reserve(2000);
    mFreeIndices.reserve(2000);
    mElements.reserve(2000);
    mFreeElementIndices.reserve(2000);
}

s32 QuadTree::allocateNode() {
    // get 4 free indices
    if (!mFreeIndices.empty()) {
        mFreeIndices.pop_back();
        mFreeIndices.pop_back();
        mFreeIndices.pop_back();
        s32 ix = mFreeIndices.back();
        mFreeIndices.pop_back();
        return ix;
    }

    // or make 4 new ones
    s32 firstChildIx = static_cast<s32>(mNodes.size());
    mNodes.emplace_back();
    mNodes.emplace_back();
    mNodes.emplace_back();
    mNodes.emplace_back();
    return firstChildIx;
}

void QuadTree::freeNode(s32 ix) {
    mNodes[ix].reset();
    mNodes[ix + 1].reset();
    mNodes[ix + 2].reset();
    mNodes[ix + 3].reset();
    mFreeIndices.push_back(ix);
    mFreeIndices.push_back(ix + 1);
    mFreeIndices.push_back(ix + 2);
    mFreeIndices.push_back(ix + 3);
}

s32 QuadTree::allocateValue() {
    if (!mFreeElementIndices.empty()) {
        s32 ix = mFreeElementIndices.back();
        mFreeElementIndices.pop_back();
        return ix;
    }
    s32 ix = static_cast<s32>(mElements.size());
    mElements.emplace_back();
    return ix;
}

void QuadTree::freeValue(Node* node, s32 valIx, s32 parentIx) {
    if (parentIx == -1) {
        // removing the first value in node's linked list
        node->firstValueIx = mElements[valIx].next;
    } else if (valIx == node->lastValueIx) {
        // removing the last value in the linked list
        mElements[parentIx].next = -1;
        node->lastValueIx = parentIx;
    } else {
        // removing a middle element
        mElements[parentIx].next = mElements[valIx].next;
    }
    // reset value in mElements and free the index
    mElements[valIx].next = -1;
    mFreeElementIndices.push_back(valIx);

    // decrement the count
    node->count--;
}

void QuadTree::pushValue(Node* node, Value value) {
    s32 newIx = allocateValue();
    if (node->firstValueIx == -1) {
        // first value added to node
        node->firstValueIx = newIx;
    } else {
        // appending
        mElements[node->lastValueIx].next = newIx;
    }
    mElements[newIx] = value;
    node->lastValueIx = newIx;
    node->count++;
}

void QuadTree::moveValue(Node* node, Value value, s32 elemIx) {
    value.next = -1;
    if (node->firstValueIx == -1) {
        // first value added to node
        node->firstValueIx = elemIx;
    } else {
        // appending
        mElements[node->lastValueIx].next = elemIx;
    }
    mElements[elemIx] = value;
    node->lastValueIx = elemIx;
    node->count++;
}

void QuadTree::add(const ecs::Entity value) {
    _add(mRootIx, 0, mBoundingBox, {value, value.get<Collider>().getShape()});
}

void QuadTree::add(const ecs::Entity value, const AABB& newShape) {
    _add(mRootIx, 0, mBoundingBox, {value, newShape});
}

void QuadTree::remove(const ecs::Entity value) {
    _remove(mRootIx, mBoundingBox, {value, value.get<Collider>().getShape()});
}
void QuadTree::remove(const ecs::Entity value, const AABB& previousShape) {
    _remove(mRootIx, mBoundingBox, {value, previousShape});
}

std::vector<ecs::Entity> QuadTree::query(const AABB& box) const {
    auto values = std::vector<ecs::Entity>();
    _query(mRootIx, mBoundingBox, box, values);
    return values;
}

AABB QuadTree::getBoundingBox() const {
    return mBoundingBox;
}

AABB QuadTree::computeBox(const AABB& box, s32 i) const {
    assert(i >= 0 && i <= 3 && "i not between 0-3");

    static const Vector2i mults[4] = {
        Vector2i(-1, 1),   // NW
        Vector2i(1, 1),    // NE
        Vector2i(-1, -1),  // SW
        Vector2i(1, -1),   // SE
    };

    const Vector2i half = (box.getHalf().as<f32>() / 2.0f).round();
    return AABB(box.getPosition() + half * mults[i], half);
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

void QuadTree::_add(s32 nodeIx, s32 depth, const AABB& parentBox, const Value value) {
    assert(nodeIx != -1);
    assert(parentBox.contains(value.shape));
    Node& node = mNodes[nodeIx];
    if (node.isLeaf()) {
        // Insert the value in this node if possible
        if (depth >= MAX_DEPTH || node.count < THRESHOLD) {
            pushValue(&node, value);
        }
        // Otherwise, we split and we try again
        else {
            split(nodeIx, parentBox);
            _add(nodeIx, depth, parentBox, value);
        }
    } else {
        const s32 i = getQuadrant(parentBox, value.shape);
        // Add the value in a child if the value is entirely contained in it
        if (i != -1) {
            _add(node.firstChildIx + i, depth + 1, computeBox(parentBox, i), value);
        }
        // Otherwise, we add the value in the current node
        else {
            pushValue(&node, value);
        }
    }
}

void QuadTree::split(const s32 nodeIx, const AABB& parentBox) {
    assert(mNodes[nodeIx].isLeaf() && "Only leaves can be split");

    // Create children
    mNodes[nodeIx].firstChildIx = allocateNode();

    // Assign values to children
    auto newValues = std::vector<Value>();  // New values for this node
    Node& node = mNodes[nodeIx];            // can hold a reference now that I'm done adding stuff
    s32 valIx = node.firstValueIx;
    // "reset" the current node's value pointers
    node.firstValueIx = -1;
    node.lastValueIx = -1;
    while (valIx > -1) {
        Value& val = mElements[valIx];
        s32 next = val.next;
        const s32 i = getQuadrant(parentBox, val.shape);
        if (i != -1)
            moveValue(&mNodes[node.firstChildIx + i], val, valIx);
        else
            moveValue(&node, val, valIx);

        valIx = next;
    }
}

bool QuadTree::_remove(s32 nodeIx, const AABB& parentBox, const Value value) {
    assert(nodeIx != -1);
    assert(parentBox.contains(value.shape));

    Node& node = mNodes[nodeIx];
    if (node.isLeaf()) {
        // Remove the value from node
        removeValue(nodeIx, value);
        return true;
    } else {
        // Remove the value in a child if the value is entirely contained in it
        const s32 i = getQuadrant(parentBox, value.shape);
        if (i != -1) {
            if (_remove(node.firstChildIx + i, computeBox(parentBox, i), value))
                return tryMerge(nodeIx);
        }
        // Otherwise, we remove the value from the current node
        else {
            removeValue(nodeIx, value);
        }

        return false;
    }
}

void QuadTree::removeValue(const s32 nodeIx, const Value value) {
    // Find the value in node.values
    Node& node = mNodes[nodeIx];
    s32 valIx = node.firstValueIx;
    s32 parentIx = -1;
    while (valIx > -1) {
        Value& val = mElements[valIx];
        if (val.entity.id() == value.entity.id()) {
            freeValue(&node, valIx, parentIx);
            return;
        }

        parentIx = valIx;
        valIx = val.next;
    }
}

bool QuadTree::tryMerge(const s32 nodeIx) {
    assert(!mNodes[nodeIx].isLeaf() && "Only interior nodes can be merged");
    u64 nbValues = mNodes[nodeIx].count;
    const s32 firstChildIx = mNodes[nodeIx].firstChildIx;
    for (s32 childIx = firstChildIx; childIx < firstChildIx + 4; childIx++) {
        const Node& child = mNodes[childIx];
        if (!child.isLeaf()) {
            return false;
        }
        nbValues += child.count;
    }
    if (nbValues <= THRESHOLD) {
        // Merge the values of all the children
        for (s32 childIx = firstChildIx; childIx < firstChildIx + 4; childIx++) {
            const Node& child = mNodes[childIx];
            s32 valIx = child.firstValueIx;
            while (valIx > -1) {
                Value& value = mElements[valIx];
                pushValue(&mNodes[nodeIx], value);
                valIx = value.next;
            }
        }
        // Remove the children
        freeNode(mNodes[nodeIx].firstChildIx);
        mNodes[nodeIx].firstChildIx = -1;  // make this a leaf node
        return true;
    } else
        return false;
}

void QuadTree::_query(const s32 nodeIx, const AABB& box, const AABB& queryBox, std::vector<ecs::Entity>& values) const {
    const Node& node = mNodes[nodeIx];
    s32 valIx = node.firstValueIx;
    while (valIx > -1) {
        const Value& value = mElements[valIx];
        if (queryBox.isOverlapping(value.shape))
            values.push_back(value.entity);

        valIx = value.next;
    }
    if (!node.isLeaf()) {
        for (s32 i = 0; i < 4; ++i) {
            const AABB childBox = computeBox(box, i);
            if (queryBox.isOverlapping(childBox))
                _query(node.firstChildIx + i, childBox, queryBox, values);
        }
    }
}

void QuadTree::_querySegment(const s32 nodeIx, const AABB& box, const Segment& segment, std::vector<ecs::Entity>& values) const {
    const Node& node = mNodes[nodeIx];
    s32 valIx = node.firstValueIx;
    while (valIx > -1) {
        const Value& value = mElements[valIx];
        if (segment.isIntersecting(value.shape))
            values.push_back(value.entity);

        valIx = value.next;
    }
    if (!node.isLeaf()) {
        for (s32 i = 0; i < 4; ++i) {
            const auto childBox = computeBox(box, static_cast<int>(i));
            if (segment.isIntersecting(childBox))
                _querySegment(node.firstChildIx + i, childBox, segment, values);
        }
    }
}

void QuadTree::_findAllIntersections(const s32 nodeIx, std::vector<std::pair<ecs::Entity, ecs::Entity>>& intersections) const {
    // Find intersections between values stored in this node
    // Make sure to not report the same intersection twice
    const Node& node = mNodes[nodeIx];
    s32 i = node.firstValueIx;
    while (true) {
        const Value& valueI = mElements[i];

        s32 j = node.firstValueIx;
        while (true) {
            const Value& valueJ = mElements[j];
            if (valueI.shape.isOverlapping(valueJ.shape)) {
                intersections.emplace_back(valueI.entity, valueJ.entity);
            }

            if (j == node.lastValueIx) {
                break;
            } else {
                j = valueJ.next;
            }
        }

        if (i == node.lastValueIx) {
            break;
        } else {
            i = valueI.next;
        }
    }

    if (node.isLeaf()) {
        // Values in this node can intersect values in descendants
        const s32 firstChildIx = node.firstChildIx;
        for (s32 childIx = firstChildIx; childIx < firstChildIx + 4; ++childIx) {
            s32 valIx = node.firstValueIx;
            while (valIx > -1) {
                const Value& value = mElements[valIx];
                _findIntersectionsInDescendants(childIx, value, intersections);

                valIx = value.next;
            }
        }
        // Find intersections in children
        for (s32 childIx = firstChildIx; childIx < firstChildIx + 4; ++childIx)
            _findAllIntersections(childIx, intersections);
    }
}

void QuadTree::_findIntersectionsInDescendants(const s32 nodeIx, const Value value,
                                               std::vector<std::pair<ecs::Entity, ecs::Entity>>& intersections) const {
    // Test against the values stored in this node
    const Node& node = mNodes[nodeIx];
    s32 valIx = node.firstValueIx;
    while (valIx > -1) {
        const Value& other = mElements[valIx];
        if (value.shape.isOverlapping(other.shape))
            intersections.emplace_back(value.entity, other.entity);

        valIx = value.next;
    }
    // Test against values stored into descendants of this node
    if (!node.isLeaf()) {
        const s32 firstChildIx = node.firstChildIx;
        for (s32 childIx = firstChildIx; childIx < firstChildIx + 4; ++childIx)
            _findIntersectionsInDescendants(childIx, value, intersections);
    }
}

RaycastHit QuadTree::raycast(Vector2f origin, Vector2f direction, f32 maxDistance, u16 layerMask) const {
    if (maxDistance == 0.0f) {
        return RaycastHit();
    }
    direction = direction.norm();
    Segment ray(origin, direction * maxDistance);
    return _raycast(ray, layerMask);
}

RaycastHit QuadTree::circlecast(Vector2f origin, Vector2f direction, f32 maxDistance, f32 radius, u16 layerMask) const {
    direction = direction.norm();
    Segment ray(origin, direction * maxDistance, radius);
    return _raycast(ray, layerMask);
}

RaycastHit QuadTree::_raycast(Segment ray, u16 layerMask) const {
    auto values = std::vector<ecs::Entity>();
    _querySegment(mRootIx, mBoundingBox, ray, values);

    RaycastHit hitinfo;
    if (values.size() == 0) {
        return hitinfo;
    }

    // Get closest entity whose collider doesn't contain the origin point
    const Vector2i originI = ray.origin.as<s32>();
    f32 closestDistance = 1e10;
    // research should i get a vector of Value structs instead so I already have the colliders?
    // NOTE: this assumes everything in the quadtree has a collider component, which i may regret later
    for (auto entity : values) {
        const auto collider = entity.get<Collider>();
        const auto shape = collider.getShape();
        if ((layerMask & collider.getLayerMask()) == 0 || shape.contains(originI)) {
            continue;
        }

        const RaycastHit curHitInfo = ray.collide(shape);
        if (curHitInfo.distance < closestDistance) {
            closestDistance = curHitInfo.distance;
            hitinfo = curHitInfo;

            // Add the entity info (has placeholder value)
            hitinfo.setOther(entity);
            hitinfo.otherMask = collider.getLayerMask();
            hitinfo.otherMaterial = collider.getMaterial();
        }
    }

    return hitinfo;
}

}  // namespace whal::qtree

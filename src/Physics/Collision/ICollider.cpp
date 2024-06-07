#include "ICollider.h"

#include <raylib.h>
#include <vector>
#include "ECS/Transform.h"
#include "Physics/Collision/Shapes.h"

namespace whal {

namespace CollisionLayer {

// this table is like Unity's Layer Collision Matrix
// Each row only considers itself and rows below it (e.g., Solid's row doesn't include Actor because Actor is above Solid)
static const std::pair<Layer, u16> LAYER_INTERACT[] = {
    {None, None},
    {Layer::Actor, Actor | Solid | SemiSolid | Trigger},
    {Layer::Solid, SemiSolid | Light | Vision},
    {Layer::SemiSolid, SemiSolid | Light | Vision},
    {Layer::Trigger, Player | Enemy | Npc},
    {Layer::Player, Enemy | Npc},  // thinking of using this for player actions? not sure
    {Layer::Enemy, None},
    {Layer::Npc, None},
    {Layer::Light, None},
    {Layer::Vision, None},
};

LayerMatrix::LayerMatrix() {
    std::vector<Layer> layers;
    for (auto [layer, mask] : LAYER_INTERACT) {
        if (layer != None) {
            layers.push_back(layer);
        }
        mInteractionTable.insert({layer, mask});
    }

    // make mappings reciprocate (idk how to spell that)
    // i.e. if Actor collides with Solid, makes sure Solid collides with Actor
    for (size_t i = 0; i < layers.size(); i++) {
        auto layer = layers[i];
        const auto mask = mInteractionTable[layer];
        for (size_t j = i + 1; j < layers.size(); j++) {
            auto otherLayer = layers[j];
            if ((mask & otherLayer) > 0) {
                // otherLayer in mask, add layer to otherLayer's mask
                mInteractionTable[otherLayer] = mInteractionTable[otherLayer] | layer;
            }
        }
    }
}

u16 LayerMatrix::getMask(Layer layer) const {
    return mInteractionTable.at(layer);
}

bool LayerMatrix::isOn(Layer layer1, Layer layer2) const {
    return (mInteractionTable.at(layer1) & layer2) > 0;
}

}  // namespace CollisionLayer

Shape::Shape(AABB aabb, CollisionLayer::Layer layer) : mAABB(aabb), mShape(ColliderShape::AABB), mLayer(layer) {}

Shape::Shape(Circle circle, CollisionLayer::Layer layer) : mCircle(circle), mShape(ColliderShape::Circle), mLayer(layer) {}

Shape::Shape(const Shape& other) {
    mShape = other.mShape;
    mLayer = other.mLayer;
    switch (other.mShape) {
    case ColliderShape::AABB:
        mAABB = other.mAABB;
        break;
    case ColliderShape::Circle:
        mCircle = other.mCircle;
        break;
    }
}

Shape& Shape::operator=(const Shape& other) {
    if (this == &other) {
        return *this;
    }
    mShape = other.mShape;
    mLayer = other.mLayer;
    switch (other.mShape) {
    case ColliderShape::AABB:
        mAABB = other.mAABB;
        break;
    case ColliderShape::Circle:
        mCircle = other.mCircle;
        break;
    }
    return *this;
}

Circle Shape::getCircle() const {
    assert(mShape == ColliderShape::Circle && "trying to run getCircle but ColliderShape is not a circle");
    return mCircle;
}

AABB Shape::getAABB() const {
    assert(mShape == ColliderShape::AABB && "trying to run getAABB but ColliderShape is not an AABB");
    return mAABB;
}

void Shape::setPosition(Vector2i center) {
    switch (mShape) {
    case ColliderShape::AABB:
        mAABB.setPosition(center);
        break;
    case ColliderShape::Circle:
        mCircle.setPosition(center);
        break;
    }
}

void Shape::setPosition(Transform2D transform) {
    switch (mShape) {
    case ColliderShape::AABB:
        mAABB.setPosition(transform);
        break;
    case ColliderShape::Circle:
        mCircle.setPosition(transform);
        break;
    }
}

Vector2i Shape::getPosition() const {
    switch (mShape) {
    case ColliderShape::AABB:
        return mAABB.getPosition();
    case ColliderShape::Circle:
        return mCircle.getPosition();
    }
}

bool Shape::isOverlapping(const Shape& other) const {
    switch (other.mShape) {
    case ColliderShape::AABB:
        return isOverlapping(other.mAABB);
    case ColliderShape::Circle:
        return isOverlapping(other.mCircle);
    }
}

bool Shape::isOverlapping(const AABB& other) const {
    switch (mShape) {
    case ColliderShape::AABB:
        return isIntersectAABBvsAABB(&mAABB, &other);
    case ColliderShape::Circle:
        return isIntersectAABBvsCircle(&other, &mCircle);
    }
}

bool Shape::isOverlapping(const Circle& other) const {
    switch (mShape) {
    case ColliderShape::AABB:
        return isIntersectAABBvsCircle(&mAABB, &other);
    case ColliderShape::Circle:
        return isIntersectCirclevsCircle(&other, &mCircle);
    }
}

void Shape::draw(Vector2f cameraPos, Color color) const {
    switch (mShape) {
    case ColliderShape::AABB:
        mAABB.draw(cameraPos, color);
        break;
    case ColliderShape::Circle:
        mCircle.draw(cameraPos, color);
        break;
    }
}

}  // namespace whal

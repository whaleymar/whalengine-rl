#pragma once

#include <unordered_map>

#include "Physics/Collision/Shapes.h"
#include "Util/Vector.h"

typedef struct Color Color;

namespace whal {

struct Transform2D;

namespace CollisionLayer {
enum Layer : u16 {
    None = 0,
    Actor = 1,
    Solid = 1 << 1,
    SemiSolid = 1 << 2,
    Trigger = 1 << 3,
    Player = 1 << 4,
    Enemy = 1 << 5,
    Npc = 1 << 6,
    Light = 1 << 7,
    Vision = 1 << 8,
};

class LayerMatrix {
public:
    LayerMatrix();

    u16 getMask(Layer layer) const;
    bool isOn(Layer layer1, Layer layer2) const;

private:
    std::unordered_map<CollisionLayer::Layer, u16> mInteractionTable;
};

}  // namespace CollisionLayer

static const CollisionLayer::LayerMatrix LAYER_MATRIX;

enum class ColliderShape : u16 { AABB, Circle };

// tagged union
class Shape {
public:
    Shape() : mAABB(Vector2i(5, 5)), mShape(ColliderShape::AABB), mLayer(CollisionLayer::None) {}

    Shape(AABB aabb, CollisionLayer::Layer layer);
    Shape(Circle circle, CollisionLayer::Layer layer);

    // apparently these get deleted bc compiler bug
    Shape(const Shape& other);
    Shape& operator=(const Shape& other);

    ColliderShape getShape() const { return mShape; }
    CollisionLayer::Layer getLayer() const { return mLayer; }

    AABB getAABB() const;
    Circle getCircle() const;
    void setPosition(Vector2i center);
    void setPosition(Transform2D transform);
    Vector2i getPosition() const;
    bool isOverlapping(const Shape& other) const;
    bool isOverlapping(const AABB& other) const;
    bool isOverlapping(const Circle& other) const;
#ifndef NDEBUG
    void draw(Vector2f cameraPos, Color color) const;
#endif

private:
    union {
        AABB mAABB;
        Circle mCircle;
    };
    ColliderShape mShape;
    CollisionLayer::Layer mLayer;
};

}  // namespace whal

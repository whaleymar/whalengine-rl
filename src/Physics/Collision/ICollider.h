#pragma once

#include <memory>
#include <unordered_map>

#include "Util/Vector.h"

typedef struct Color Color;

namespace whal {

struct Transform2D;

enum class ColliderShape : u16 { AABB, Circle };

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

class IColliderShape {
public:
    IColliderShape(Vector2i center, ColliderShape shape, CollisionLayer::Layer layer) : mCenter(center), mShape(shape), mLayer(layer) {}
    // virtual ~IColliderShape() = default;

    void setPosition(Vector2i center) { mCenter = center; }
    Vector2i getPosition() const { return mCenter; }
    ColliderShape getShape() const { return mShape; }
    CollisionLayer::Layer getLayer() const { return mLayer; }

    // virtual void setPosition(Transform2D transform) = 0;
    virtual bool isOverlapping(const IColliderShape* other) const = 0;
    // virtual std::unique_ptr<IColliderShape> clone() const = 0;
#ifndef NDEBUG
    virtual void draw(Vector2f cameraPos, Color color) const = 0;
#endif

protected:
    Vector2i mCenter;
    ColliderShape mShape;
    CollisionLayer::Layer mLayer;
};

}  // namespace whal

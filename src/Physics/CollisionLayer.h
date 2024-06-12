#pragma once

#include <unordered_map>
#include "Util/Types.h"

typedef struct Color Color;

namespace whal {

struct Transform2D;

namespace CollisionLayer {
enum Layer : u16 {
    None = 0,
    Actor = 1,
    Solid = 1 << 1,
    SemiSolid = 1 << 2,
    Trigger = 1 << 3,  // TODO TriggerActors
    Player = 1 << 4,
    Enemy = 1 << 5,
    Npc = 1 << 6,
    Light = 1 << 7,
    Vision = 1 << 8,
};

constexpr u16 ALL = 0xffff;
constexpr u16 PHYSICS = Actor | Solid | SemiSolid;
constexpr u16 SOLID = Solid | SemiSolid;

class LayerMatrix {
public:
    LayerMatrix();

    u16 getMask(Layer layer) const { return mInteractionTable.at(layer); }

    bool isOn(Layer layer1, Layer layer2) const { return (mInteractionTable.at(layer1) & layer2) > 0; }

    bool isPhysicsLayer(Layer layer) const { return layer & CollisionLayer::PHYSICS; }

    bool isSolidAny(Layer layer) const { return layer & CollisionLayer::SOLID; }

private:
    std::unordered_map<CollisionLayer::Layer, u16> mInteractionTable;
};

}  // namespace CollisionLayer

static const CollisionLayer::LayerMatrix LAYER_MATRIX;

}  // namespace whal

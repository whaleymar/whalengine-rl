#pragma once

#include <unordered_map>
#include "Util/Types.h"

namespace whal {

struct Transform;

namespace CollisionLayer {
enum Layer : u16 {
    None = 0,
    Actor = 1,
    Solid = 1 << 1,
    SemiSolid = 1 << 2,
    TriggerPhysics = 1 << 3,
    TriggerActors = 1 << 4,
    PlayerFriendlyFire = 1 << 5,  // interacts with solids and itself, but not actors. Good for player projectiles that spawn inside the player.

    // These are basically tags that affect the physics system (think raycasts - maybe you want to check if something will hit an enemy)
    Player = 1 << 6,
    Enemy = 1 << 7,
    Npc = 1 << 8,
    Attack = 1 << 9,
    Light = 1 << 10,
    BlocksVision = 1 << 11,
};

Layer fromString(const char* layer);

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

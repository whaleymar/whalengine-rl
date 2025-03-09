#pragma once

#include <unordered_map>
#include "Util/Types.h"

namespace whal {

// Different physics interactions between each PhysicsBody
// Heavy Body has infinite weight. PushesRigid Body and Feather Body, passes through other Heavy Bodyies.
// Rigid Body pushes other rigid bodies. Pushes Feather bodies.
// Feather Body doesn't push anything.
enum class PhysicsBody : u16 {
    Feather,  // formerly `Actor`
    Rigid,    // formerly `SemiSolid`
    Heavy,    // formerly `Solid`
};

struct Transform;

namespace CollisionLayer {
enum Layer : u16 {
    None = 0,
    ActorPhysics = 1,
    DefaultPhysics = 1 << 1,
    PhysicsNoActor = 1 << 2,  // interacts with solids and itself, but not actors. Good for player projectiles that spawn inside the player.

    // These are basically tags that affect the physics system (think raycasts - maybe you want to check if something will hit an enemy)
    Player = 1 << 3,
    Enemy = 1 << 4,
    Npc = 1 << 5,
    Attack = 1 << 6,
    BlocksVision = 1 << 7,
    BlocksMovement = 1 << 8,
};

Layer fromString(const char* layer);

constexpr u16 ALL = 0xffff;
constexpr u16 PHYSICS = ActorPhysics | DefaultPhysics;
constexpr u16 SOLID = DefaultPhysics;

class LayerMatrix {
public:
    LayerMatrix();

    u16 getMask(Layer layer) const { return mInteractionTable.at(layer); }
    u16 getMask(u16 mask) const;  // gets interact mask for each layer in `mask`
    bool isOn(Layer layer1, Layer layer2) const { return (mInteractionTable.at(layer1) & layer2) > 0; }
    bool isPhysicsLayer(Layer layer) const { return layer & CollisionLayer::PHYSICS; }
    bool isSolidAny(Layer layer) const { return layer & CollisionLayer::SOLID; }

private:
    std::unordered_map<CollisionLayer::Layer, u16> mInteractionTable;
};

}  // namespace CollisionLayer

static const CollisionLayer::LayerMatrix LAYER_MATRIX;

}  // namespace whal

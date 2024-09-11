#include "CollisionLayer.h"

#include <vector>
#include "Util/String.h"

#define RETURN_IF(var, name)                                                                                                                         \
    if (isEqualString(#name, var))                                                                                                                   \
    return name

namespace whal {

namespace CollisionLayer {

Layer fromString(const char* layer) {
    RETURN_IF(layer, None);
    RETURN_IF(layer, Actor);
    RETURN_IF(layer, Solid);
    RETURN_IF(layer, SemiSolid);
    RETURN_IF(layer, TriggerPhysics);
    RETURN_IF(layer, TriggerActors);
    RETURN_IF(layer, Player);
    RETURN_IF(layer, Enemy);
    RETURN_IF(layer, Npc);
    RETURN_IF(layer, Light);
    RETURN_IF(layer, Vision);

    return None;
}

// This table is like Unity's Layer Collision Matrix.
// It's symmetric, so each row only considers itself and rows below it.
// For example, the Solid layer's row doesn't include Actor because Actor is above Solid.
// The Actor layer's line includes Solid, meaning they interact with each other.
static const std::pair<Layer, u16> LAYER_INTERACT[] = {
    {None, None},
    {Layer::Actor, Solid | SemiSolid | TriggerActors | TriggerPhysics},
    {Layer::Solid, SemiSolid | Light | Vision | TriggerPhysics | PlayerFriendlyFire | Player | Enemy | Npc},
    {Layer::SemiSolid, SemiSolid | Light | Vision | TriggerPhysics | PlayerFriendlyFire | Player | Enemy | Npc},
    {Layer::TriggerPhysics, None},
    {Layer::TriggerActors, None},
    {Layer::PlayerFriendlyFire, PlayerFriendlyFire | Player | Enemy | Npc},
    {Layer::Player, Enemy | Npc},  // thinking of using this for player actions? not sure
    {Layer::Enemy, None},
    {Layer::Npc, None},
    {Layer::Light, None},
    {Layer::Vision, None},
};

LayerMatrix::LayerMatrix() {
    // RESEARCH could use bit tricks to convert a layer mask to an array index & avoid the dictionary
    // https://stackoverflow.com/questions/71539154/convert-power-of-2-bitmask-into-corresponding-index
    std::vector<Layer> layers;
    for (auto [layer, mask] : LAYER_INTERACT) {
        if (layer != None) {
            layers.push_back(layer);
        }
        mInteractionTable.insert({layer, mask});
    }

    // make mappings symmetric
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

}  // namespace CollisionLayer

}  // namespace whal

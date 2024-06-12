#include "CollisionLayer.h"

#include <vector>

namespace whal {

namespace CollisionLayer {

// This table is like Unity's Layer Collision Matrix.
// It's symmetric, so each row only considers itself and rows below it.
// For example, the Solid layer's row doesn't include Actor because Actor is above Solid.
// The Actor layer's line includes Solid, meaning they interact with each other.
static const std::pair<Layer, u16> LAYER_INTERACT[] = {
    {None, None},
    {Layer::Actor, Solid | SemiSolid | Trigger},
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

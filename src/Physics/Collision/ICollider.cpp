#include "ICollider.h"
#include <vector>

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

}  // namespace whal

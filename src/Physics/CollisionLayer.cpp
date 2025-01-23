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
    RETURN_IF(layer, ActorPhysics);
    RETURN_IF(layer, DefaultPhysics);
    RETURN_IF(layer, Player);
    RETURN_IF(layer, Enemy);
    RETURN_IF(layer, Npc);
    RETURN_IF(layer, BlocksVision);

    return None;
}

// This table is like Unity's Layer Collision Matrix.
// It's symmetric, so each row only considers itself and rows below it.
// For example, the Solid layer's row doesn't include Actor because Actor is above Solid.
// The Actor layer's line includes Solid, meaning they interact with each other.
static const std::pair<Layer, u16> LAYER_INTERACT[] = {
    {None, None},
    {Layer::ActorPhysics, DefaultPhysics},
    {Layer::DefaultPhysics, DefaultPhysics | PhysicsNoActor | Attack | BlocksVision},
    {Layer::PhysicsNoActor, PhysicsNoActor | Attack},
    {Layer::Player, Attack},
    {Layer::Enemy, Attack},
    {Layer::Npc, None},
    {Layer::Attack, Attack},
    {Layer::BlocksVision, None},
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

u16 LayerMatrix::getMask(u16 mask) const {
    u16 result = 0;
    for (s32 i = 0; i < 16; i++) {
        // check each bit
        Layer bitmask = static_cast<Layer>(1 << i);
        if ((mask & bitmask) > 0) {
            result |= getMask(bitmask);
        }
    }
    return result;
}

}  // namespace CollisionLayer

}  // namespace whal

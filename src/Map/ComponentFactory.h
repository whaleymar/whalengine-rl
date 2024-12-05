#pragma once

#include <unordered_map>
#include "json_fwd.hpp"

#include "Util/ISerialize.h"
#include "Util/Types.h"

namespace whal {

namespace ecs {
class Entity;
}
struct LayerData;
struct ActiveLevel;
struct EntityMapData;
struct Follow;

// passed as const reference when loading components
struct LoadContext {
    const nlohmann::json& values;  // should rename this to `componentData` or something
    const nlohmann::json& allObjects;
    const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex;
    const EntityMapData& entityData;
    const ActiveLevel& level;
    const LayerData& layerData;
    bool isTiledData = false;
};

// loadImpl stub:
// void ::loadImpl(ecs::Entity entity, void* data) {
//     const LoadContext& ctx = *static_cast<LoadContext*>(data);
// }
struct ComponentFactory : SerializeFactory<ComponentFactory> {
    template <typename T>
    static void DefaultLoadImpl(ecs::Entity entity, void* data) {
        // print("Running ComponentFactoryNew::DefaultLoadImpl for type:", type_of<T>());
        // const LoadContext& ctx = *static_cast<LoadContext*>(data);
    }

    template <typename T>
    static void* DefaultSaveImpl(ecs::Entity entity) {
        // print("Running ComponentFactoryNew::DefaultSaveImpl");
        return nullptr;
    }
};

}  // namespace whal

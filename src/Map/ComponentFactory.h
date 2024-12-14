#pragma once

#include <type_traits>
#include <unordered_map>
#include "json_fwd.hpp"

#include "Map/TiledParse.h"
#include "Util/ISerialize.h"
#include "Util/Types.h"
#include "rfl/to_view.hpp"

namespace whal {

namespace ecs {
class Entity;
}

struct ActiveLevel;
struct EntityMapData;
struct Follow;
struct PropertyType;
enum class TiledDataType;

// passed as const reference when loading components
struct LoadContext {
    const nlohmann::json& values;  // should rename this to `componentData` or something
    const nlohmann::json& allObjects;
    const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex;
    const EntityMapData& entityData;
    const ActiveLevel& level;
    bool isTiledData = false;
};

// loadImpl stub:
// void ::loadImpl(ecs::Entity entity, void* data) {
//     const LoadContext& ctx = *static_cast<LoadContext*>(data);
// }
struct ComponentFactory : SerializeFactory<ComponentFactory> {
    // Requirements for the Default Loader:
    // 1. The component does not have a custom constructor
    // 2. (tiled specific) the tiled property types and members are named exactly the same as in code
    template <typename T>
        requires(CustomLoad<T> ||
                 std::is_aggregate<T>::value)  //  ComponentFactory::DefaultLoadImpl doesn't work for components with custom constructors
    static void DefaultLoadImpl(ecs::Entity entity, void* data) {
        const LoadContext& ctx = *static_cast<LoadContext*>(data);

        T cpnt = entity.has<T>() ? entity.get<T>() : T{};
        if (ctx.isTiledData) {
            // TODO this doesn't handle a couple of things:
            // 1. parsing enum from string
            // 2. parsing shape
            // 3. parsing target entity ID

            const auto view = rfl::to_view(cpnt);
            view.apply([&](const auto& f) { tryRead(ctx.values, f.name(), f.value()); });
        }

        entity.add(cpnt);
    }

    template <typename T>
    static void* DefaultSaveImpl(ecs::Entity entity) {
        // print("Running ComponentFactoryNew::DefaultSaveImpl");
        return nullptr;
    }

    // TODO these are currently UNUSED. They probably have more information than I need for parsing enums

    // maps a Tiled type name to a struct with metadata about that type
    static std::unordered_map<std::string, PropertyType> propertyTypes;

    // maps a Tiled class member's name to its type.
    // name is qualified, so Property "Light" with member "radius" will form the key "Light:radius"
    // if the type is a class, we also need to store the propertyType of that class
    static std::unordered_map<std::string, std::pair<TiledDataType, std::string>> memberTypes;
};

}  // namespace whal

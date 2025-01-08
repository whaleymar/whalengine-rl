#pragma once

#include <type_traits>
#include <unordered_map>
#include "json_fwd.hpp"

#include <rfl/json.hpp>
#include "Map/TiledParse.h"
#include "Util/ISerialize.h"
#include "Util/Types.h"
#include "rfl/to_view.hpp"

namespace whal {

struct ActiveLevel;
struct EntityMapData;
struct Follow;
struct PropertyType;
enum class TiledDataType;

// passed as const reference when loading components
struct LoadContext {
    const nlohmann::json* values;  // should rename this to `componentData` or something
    const nlohmann::json& allObjects;
    const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex;
    const EntityMapData& entityData;
    ecs::Entity self;
    ecs::Entity parent;
    bool isTiledData = false;
};

// not working...
// template <typename T, typename = void>
// struct IsSerializable : std::false_type {};
// template <typename T>
// struct IsSerializable<T, std::void_t<decltype(rfl::json::write(std::declval<T>()))>> : std::true_type {};

// loadImpl signature:
// void ::loadImpl(ecs::Entity entity, const LoadContext& ctx);
struct ComponentFactory : SerializeFactory<ComponentFactory, LoadContext> {
    // Requirements for the Default Loader:
    // 1. The component does not have a custom constructor
    // 2. (tiled specific) the tiled property types and members are named exactly the same as in code
    template <typename T>
        requires(CustomLoad<T, LoadType> ||
                 std::is_aggregate<T>::value)  //  ComponentFactory::DefaultLoadImpl doesn't work for components with custom constructors
    static void DefaultLoadImpl(ecs::Entity entity, const LoadContext& ctx) {
        T cpnt = entity.has<T>() ? entity.get<T>() : T{};
        if (ctx.isTiledData) {
            // TODO this doesn't handle a couple of things:
            // 1. parsing enum from string
            // 2. parsing shape
            // 3. parsing target entity ID

            const auto view = rfl::to_view(cpnt);
            view.apply([&](const auto& f) { tryRead(*ctx.values, f.name(), f.value()); });
        }

        entity.add(cpnt);
    }

    template <typename T>
    // requires(IsSerializable<T>())
    static std::string DefaultSaveImpl(ecs::Entity entity) {
        // print("Running ComponentFactoryNew::DefaultSaveImpl");
        // std::string data = rfl::json::write(entity.get<T>());
        // print(data);
        return nullptr;
    }

    // template <typename T>
    // requires(!IsSerializable<T>())
    // static void* DefaultSaveImpl(ecs::Entity entity) {
    //     return nullptr;
    // }

    // TODO these are currently UNUSED. I would like to use them to automatically deserialize Tiled property members based on their type.
    //    Right now, I dispatch an overloaded `tryRead` call to set each member when deserializing a component.
    //    Instead, I want to check what type the member is in tiled (is it an enum? is it a Tiled color? A whal::Color?)
    //    And use that information to deserialize without writing a dedicated `tryRead` implementation
    //    I am still unsure how to go about writing this deserialization function.
    //    Like, if I am deserializing a component with a whal::Color member, it should handle the default color type and a custom whal::Color type
    //    So if I look up ComponentName:color in memberTypes, I'll see either {Color, ""} or {Class, "whal::Color"}
    //    For the former, I would use the existing `tryRead<whal::Color>` impl which is designed to parse a Tiled color
    //    For the latter, ???. I can't register a custom parser in the ComponentFactory because it's not a component...
    //        If I make it match the c++ struct exactly (i.e. 4 floats, not Tiled color + brightness float), I could maybe load it with rfl
    //        I could create a new factory with a signature matching `tryRead` to implement custom parsing logic
    //        => That's probably the solution. Create a JsonFactory that inherits SerializeFactory and give it a default loader which uses rfl, and it
    //        basically replaces all the `tryRead` overloads
    //        => TBH, this would only be *slightly* more convenient than writing overloads for `tryRead`. It comes with the benefit of custom parsers
    //        depending on the Tiled type (rather than the c++ struct type), but at the cost of compile time

    // maps a Tiled type name to a struct with metadata about that type
    static std::unordered_map<std::string, PropertyType> propertyTypes;

    // maps a Tiled class member's name to its type.
    // name is qualified, so Property "Light" with member "radius" will form the key "Light:radius"
    // if the type is a class, we also need to store the propertyType of that class as a string,
    // So if Light:color is a whal::Color in Tiled, then the value is {TiledDataType::Class, "whal::Color"}
    // TODO should use PropertyType instead of TiledDataType
    static std::unordered_map<std::string, std::pair<TiledDataType, std::string>> memberTypes;
};

}  // namespace whal

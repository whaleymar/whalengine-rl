#pragma once

#include <unordered_map>
#include "ECS.h"

#include "Util/JsonDoc.h"
#include "Util/Types.h"

namespace whal {

struct ActiveLevel;
struct EntityMapData;
struct PropertyType;
enum class TiledDataType;

// passed as const reference when loading components
struct LoadContext {
    JsonValue values;  // should rename this to `componentData` or something
    JsonValue allObjects;
    const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex;
    const EntityMapData& entityData;
    ecs::Entity self;
    ecs::Entity parent;
};

// a meta-component (given to other components) with a callback that parses tiled data and adds the component to the entity.
struct TiledDeserialize {
    using Loader = void (*)(ecs::Entity, const LoadContext&);
    Loader load;
};

class ComponentFactory {
public:
    static const TiledDeserialize* get(const char* componentName);
    static void init();

    // These are currently UNUSED. I would like to use them to automatically deserialize Tiled property members based on their type.
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

private:
    static void initTiledLoader();
    static void initEcsSerializer();
};

}  // namespace whal

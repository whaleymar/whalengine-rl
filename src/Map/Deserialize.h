// An attempted proof-of-concept for a generic reflection-based component deserializer
// Couldn't make it work easily, unfortunately.
// I think it *could* work if I can save some string->type mapping when rfl::fields<T>() runs
// and then use that in rfl::make_field.
// BUT this would only work for components made of core types -- things like entity relationships and map relationships
// would still need manual intervention

// #pragma once
//
// #include "Map/Tiled.h"
//
// #include "json.hpp"
//
// #include <rfl.hpp>
// #include "rfl/fields.hpp"
// #include "rfl/replace.hpp"
//
// namespace whal {
//
// template <typename T>
// void addComponentMembers(const nlohmann::json& values, const nlohmann::json& allObjects,
//                          const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, const ActiveLevel& level,
//                          ecs::Entity entity, LayerData layerData) {
//     T component = T{};
//
//     for (const auto& f : rfl::fields<T>()) {
//         if (!values.contains(f.name())) {
//             continue;
//         }
//
//         // this doesn't work, because rfl::make_field deduces its return type based on arg1, but the json struct is type-erased
//         component = rfl::replace(component, rfl::make_field<f.name()>(values[f.name()]));
//     }
//
//     entity.add(component);
// }
//
// }  // namespace whal

#pragma once

#include <rfl/json.hpp>
#include "ECS.h"
#include "Map/ComponentFactory.h"
#include "Serializer.h"
#include "Sys/System.h"
#include "rfl/to_view.hpp"

namespace whal {

// Convenience functions/macros for adding Serializers to components

///////////////
// ECS STUFF //
///////////////

template <typename T>
static ecs::Serialize defaultSerdeImpl() {
    return {
        .ser = [](ecs::Entity e) -> std::string { return rfl::json::write(e.get<T>()); },
        .de =
            [](ecs::Entity e, std::string data) {
                auto dataOpt = rfl::json::read<T>(data);
                if (dataOpt) {
                    e.add<T>(*dataOpt);
                } else {
                    print("Error parsing component: ", dataOpt.error()->what());
                    e.add<T>();
                }
            },
    };
}

template <typename T>
static ecs::Serialize tagSerdeImpl() {
    return {
        .ser = nullptr,
        .de = [](ecs::Entity e, std::string data) { e.add<T>(); },
    };
}

template <typename T>
static void addDefault() {
    ecs::Entity e = World.component<T>();
    if (e.has<ecs::internal::Tag>()) {
        e.add<ecs::Serialize>(tagSerdeImpl<T>());
    } else {
        e.add<ecs::Serialize>(defaultSerdeImpl<T>());
    }
}

/////////////////
// TILED STUFF //
/////////////////

template <typename T>
static TiledDeserialize defaultTiledLoaderImpl() {
    return {
        .load =
            [](ecs::Entity entity, const LoadContext& ctx) {
                T cpnt = entity.has<T>() ? entity.get<T>() : T{};
                // this doesn't handle a couple of things:
                // 1. parsing enum from string
                // 2. parsing shape
                // 3. parsing target entity ID

                // for enum and shape, I can use this pattern to figure out the field type and dispatch a different overload of tryRead:

                // const auto tup = rfl::to_view(cpnt);
                // tup.apply([&]<typename Field>(Field& f) {
                //     using Dtype = std::remove_reference_t<decltype(*f.value())>;

                const auto view = rfl::to_view(cpnt);
                view.apply([&](const auto& f) { tryRead(*ctx.values, f.name(), f.value()); });

                entity.add(cpnt);
            },
    };
}

template <typename T>
static TiledDeserialize tagTiledLoaderImpl() {
    return {
        .load = [](ecs::Entity e, const LoadContext& ctx) { e.add<T>(); },
    };
}

template <typename T>
static void addDefaultTiledLoader() {
    ecs::Entity e = World.component<T>();
    if (e.has<ecs::internal::Tag>()) {
        e.add<TiledDeserialize>(tagTiledLoaderImpl<T>());
    } else {
        e.add<TiledDeserialize>(defaultTiledLoaderImpl<T>());
    }
}

// #define LOADER_DEF(name) void load##name(ecs::Entity, const LoadContext&)

#define TILED_LOADER(name, def)                                                                                                                      \
    World.component<name>().add<TiledDeserialize>({                                                                                                  \
        .load = [](ecs::Entity entity, const LoadContext& ctx) { def },                                                                              \
    })

}  // namespace whal

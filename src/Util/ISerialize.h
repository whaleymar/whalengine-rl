#pragma once

#include <optional>
#include "ECS.h"
#include "Traits.h"
#include "TypeName.h"

namespace whal {

template <typename Key, typename Value>
struct Map {
    std::vector<std::pair<Key, Value>> data;
    size_t size = 0;

    bool insert(const Key& key, const Value& val) {
        data.push_back({key, val});
        ++size;
        return true;
    }

    [[nodiscard]] std::optional<Value> at(const Key& key) const {
        for (auto [k, v] : data) {
            if (k == key) {
                return v;
            }
        }
        return std::nullopt;
    }
};

// Pass Self as a template parameter so each implementation gets its own lookup table
template <typename Self, typename LoadContextType, typename SaveContextType = std::string>
struct SerializeFactory {
    using LoadType = LoadContextType;
    using SaveType = SaveContextType;
    using LoadMethod = void (*)(ecs::Entity, const LoadContextType&);
    using SaveMethod = SaveType (*)(ecs::Entity);
    struct SerializeFuncs {
        LoadMethod load;
        SaveMethod save;
    };

    SerializeFactory() = delete;

    static bool Register(std::string_view name, LoadMethod loader, SaveMethod saver) {
        return mFactoryTable.insert(name, SerializeFuncs{loader, saver});
    }
    static std::optional<SerializeFuncs> Get(std::string_view name) { return mFactoryTable.at(name); }

    template <typename T>
    static void DefaultLoad(ecs::Entity entity, const LoadContextType& data) {
        return Self::template DefaultLoadImpl<T>(entity, data);
    }

    template <typename T>
    static SaveType DefaultSave(ecs::Entity entity) {
        return Self::template DefaultSaveImpl<T>(entity);
    }

private:
    // constinit is necessary to guarantee the factory exists before any static variables are initialized (otherwise it would be UB)
    static inline constinit Map<std::string_view, SerializeFuncs> mFactoryTable;
};

///////////////////////////////////////////////////////////////////////////
//////////////////////////   TRAITS  //////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

// checks that T::LoadImpl(ecs::Entity, const LoadContextType&) exists and returns nothing
template <typename T, typename LoadContextType>
concept CustomLoad = requires {
    { T::loadImpl(ecs::Entity{}, std::declval<const LoadContextType&>()) } -> std::same_as<void>;
};

// checks that T::SaveImpl(ecs::Entity) exists and returns an opaque pointer
template <typename T, typename SaveType>
concept CustomSave = requires {
    { T::saveImpl(ecs::Entity{}) } -> std::same_as<SaveType>;
};

// These concepts ensure a factory has a "DefaultLoadImpl" method and a "DefaultSaveImpl" method (static functions)
template <typename T, typename LoadContextType>
concept DefaultLoad = requires {
    { T::DefaultLoadImpl(ecs::Entity{}, LoadContextType{}) } -> std::same_as<void>;
};

template <typename T, typename SaveType>
concept DefaultSave = requires {
    { T::DefaultSaveImpl(ecs::Entity{}) } -> std::same_as<SaveType>;
};

// 1. Factory Inherits from SerializeFactory
// 2. Factory Implements static DefaultLoadImpl method
// 3. Factory Implements static DefaultSaveImpl method
template <typename T>
concept IsValidFactory =
    requires { is_base_of_template<SerializeFactory, T>::value&& DefaultLoad<T, typename T::LoadType>&& DefaultSave<T, typename T::SaveType>; };

///////////////////////////////////////////////////////////////////////////
////////////////////////// INTERFACE //////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

template <typename T, class Factory>
    requires IsValidFactory<Factory>
struct ISerialize {
    static void forceInit() {
        (void)S_IS_REGISTERED;  // force the compiler to initialize S_IS_REGISTERED
    }

    // By calling forceInit here, it's guaranteed that T is registered in the factory before the program starts.
    ISerialize() { forceInit(); }

    static void load(ecs::Entity entity, const Factory::LoadType& data) {
        if constexpr (CustomLoad<T, typename Factory::LoadType>) {
            T::loadImpl(entity, data);
        } else {
            Factory::template DefaultLoad<T>(entity, data);
        }
    }

    static Factory::SaveType save(ecs::Entity entity) {
        if constexpr (CustomSave<T, typename Factory::SaveType>) {
            return T::saveImpl(entity);
        } else {
            return Factory::template DefaultSave<T>(entity);
        }
    }

    // type_of<T>() gets the name of the type (with namespacing)
    static inline const bool S_IS_REGISTERED = Factory::Register(type_of<T>(), load, save);
};

}  // namespace whal

// This macro manually registers a type with its factory.
// This is only necessary when the type is never constructed anywhere in the program.
// Otherwise, registration is automatic.
#define REGISTER_SERIALIZE(type) static inline const bool S_INITFLAG_##type = type::S_IS_REGISTERED;

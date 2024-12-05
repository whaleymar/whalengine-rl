#pragma once

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

using LoadMethod = void (*)(ecs::Entity, void*);
using SaveMethod = void* (*)(ecs::Entity);
struct SerializeFuncs {
    LoadMethod load;
    SaveMethod save;
};

// Pass Self as a template parameter so each implementation gets its own lookup table
template <typename Self>
struct SerializeFactory {
    SerializeFactory() = delete;

    static bool Register(std::string_view name, SerializeFuncs creatorFuncs) { return mFactoryTable.insert(name, creatorFuncs); }
    static std::optional<SerializeFuncs> Get(std::string_view name) { return mFactoryTable.at(name); }

    template <typename T>
    static void DefaultLoad(ecs::Entity entity, void* data) {
        return Self::template DefaultLoadImpl<T>(entity, data);
    }

    template <typename T>
    static void* DefaultSave(ecs::Entity entity) {
        return Self::template DefaultSaveImpl<T>(entity);
    }

private:
    // constinit is necessary to guarantee the factory exists before any static variables are initialized (otherwise it would be UB)
    static inline constinit Map<std::string_view, SerializeFuncs> mFactoryTable;
};

///////////////////////////////////////////////////////////////////////////
//////////////////////////   TRAITS  //////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

// checks that T::LoadImpl(ecs::Entity, void*) exists and returns nothing
template <typename T>
concept CustomLoad = requires {
    { T::loadImpl(ecs::Entity{}, (void*)nullptr) } -> std::same_as<void>;
};

// checks that T::SaveImpl(ecs::Entity) exists and returns an opaque pointer
template <typename T>
concept CustomSave = requires {
    { T::saveImpl(ecs::Entity{}) } -> std::same_as<void*>;
};

template <typename T>
concept DefaultLoad = requires {
    { T::DefaultLoadImpl(ecs::Entity{}, (void*)nullptr) } -> std::same_as<void>;
};

template <typename T>
concept DefaultSave = requires {
    { T::DefaultSaveImpl(ecs::Entity{}) } -> std::same_as<void*>;
};

// 1. Factory Inherits from SerializeFactory
// 2. Factory Implements static DefaultLoadImpl method
// 3. Factory Implements static DefaultSaveImpl method
template <typename T>
concept IsValidFactory = requires { is_base_of_template<SerializeFactory, T>::value&& DefaultLoad<T>&& DefaultSave<T>; };

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

    static void load(ecs::Entity entity, void* data) {
        if constexpr (CustomLoad<T>) {
            T::loadImpl(entity, data);
        } else {
            Factory::template DefaultLoad<T>(entity, data);
        }
    }

    static void* save(ecs::Entity entity) {
        if constexpr (CustomSave<T>) {
            return T::saveImpl(entity);
        } else {
            return Factory::template DefaultSave<T>(entity);
        }
    }

    // type_of<T>() gets the name of the type (with namespacing)
    static inline const bool S_IS_REGISTERED = Factory::Register(type_of<T>(), SerializeFuncs{load, save});
};

}  // namespace whal

// This macro manually registers a type with its factory.
// This is only necessary when the type is never constructed anywhere in the program.
// Otherwise, registration is automatic.
#define REGISTER_SERIALIZE(type) static inline const bool S_INITFLAG_##type = type::S_IS_REGISTERED;

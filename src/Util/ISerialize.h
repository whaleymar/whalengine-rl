#pragma once

#include <type_traits>
#include "ECS.h"
#include "STL_reduce.h"
#include "TypeName.h"

namespace whal {

// Key-Value map with constexpr constructor and methods.
// From https://www.cppstories.com/2023/ub-factory-constinit/
// I don't think the methods are actually constexpr, it doesn't make sense to me and the compiler hates them
// they *might* work in c++23, which would be nice because I can avoid the macro and make the ISerialize registration use constinit
template <typename Key, typename Value, size_t Size>
struct Map {
    std::array<std::pair<Key, Value>, Size> data;
    size_t slot_{0};

    // constexpr bool insert(const Key& key, const Value& val) {
    bool insert(const Key& key, const Value& val) {
        if (slot_ < Size) {
            data[slot_] = std::make_pair(key, val);
            ++slot_;
            return true;
        }
        return false;
    }

    // [[nodiscard]] constexpr std::optional<Value> at(const Key& key) const {
    [[nodiscard]] std::optional<Value> at(const Key& key) const {
        const auto itr = stl::find_if(begin(data), end(data), [&key](const auto& v) { return v.first == key; });
        if (itr != end(data)) {
            return itr->second;
        } else {
            return std::nullopt;
        }
    }
};

using LoadMethod = void (*)(ecs::Entity, void*);
using SaveMethod = void* (*)(ecs::Entity);
struct SerializeFuncs {
    LoadMethod load;
    SaveMethod save;
};

// so I can use is_base_of:
struct ISerializeFactory {};

// Pass Self as a template parameter so each implementation gets its own lookup table
template <typename Self, size_t N>
struct SerializeFactory : ISerializeFactory {
    SerializeFactory() = delete;

    static constexpr bool Register(std::string_view name, SerializeFuncs creatorFuncs) { return mFactoryTable.insert(name, creatorFuncs); }
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
    static inline constinit Map<std::string_view, SerializeFuncs, N> mFactoryTable;
    // TODO add a static unordered_map member which is populated by mFactoryTable once .finish() gets called
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
concept IsValidFactory = requires { std::is_base_of_v<ISerializeFactory, T>&& DefaultLoad<T>&& DefaultSave<T>; };

///////////////////////////////////////////////////////////////////////////
////////////////////////// INTERFACE //////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

template <typename T, class Factory>
    requires IsValidFactory<Factory>
struct ISerialize {
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

#pragma once

#include "ECS.h"

namespace whal {

template <typename T>
void DefaultSave(ecs::Entity entity, const T& data);

template <typename T>
void DefaultLoad(ecs::Entity entity);

template <typename T, bool UseDefault = true>
struct ISerialize;

template <typename T, bool UseDefault>
struct ISerialize {
    static void Load(ecs::Entity entity) {
        if constexpr (UseDefault) {
            DefaultLoad<T>(entity);
        } else {
            T::LoadImpl(entity);
        }
    }

    void Save(ecs::Entity entity) const {
        if constexpr (UseDefault) {
            DefaultSave<T>(entity, *this);
        } else {
            static_cast<T*>(this)->Save(entity);
        }
    }
};

}  // namespace whal

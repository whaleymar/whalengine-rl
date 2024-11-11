#pragma once

#include "Map/ComponentFactory.h"
#include "Map/EntityFactory.h"
namespace whal {

struct System;

class PrefabManager {
public:
    friend System;

    PrefabManager() = default;

    EntityFactory entity;
    ComponentFactory component;

private:
    PrefabManager(const PrefabManager&) = delete;
    void operator=(const PrefabManager&) = delete;
};

}  // namespace whal

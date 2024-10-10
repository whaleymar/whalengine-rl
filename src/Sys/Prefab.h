#pragma once

#include "Map/ComponentFactory.h"
#include "Map/EntityFactory.h"
namespace whal {

struct System;

class Prefab {
public:
    friend System;

    EntityFactory entity;
    ComponentFactory component;

private:
    Prefab() = default;
    Prefab(const Prefab&) = delete;
    void operator=(const Prefab&) = delete;
};

}  // namespace whal

#pragma once

#include "Map/EntityFactory.h"

namespace whal {

struct System;

class PrefabManager {
public:
    friend System;

    PrefabManager() = default;

    EntityFactory entity;

private:
    PrefabManager(const PrefabManager&) = delete;
    void operator=(const PrefabManager&) = delete;
};

}  // namespace whal

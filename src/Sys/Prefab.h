#pragma once

#include "Map/EntityFactory.h"
#include "Util/Singleton.h"

namespace whal {

struct System;

class PrefabManager {
    SINGLETON(PrefabManager)
public:
    EntityFactory entity;
};

}  // namespace whal

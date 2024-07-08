#pragma once

namespace whal::ecs {
using EntityID = u32;
}

struct Switch {
    whal::ecs::EntityID target;
};

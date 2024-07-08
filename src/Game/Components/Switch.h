#pragma once

namespace whal::ecs {

using EntityID = u32;
// class Entity;

}  // namespace whal::ecs

struct Switch {
    whal::ecs::EntityID target;
};

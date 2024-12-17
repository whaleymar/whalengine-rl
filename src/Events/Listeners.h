#pragma once

namespace whal {
namespace ecs {
class Entity;
}

// These callbacks are registered with the ECS World

// This runs when an entity dies
void emitEntityDeathEvent(ecs::Entity entity);

// This runs when an entity is created without a parent. It adds a default Transform component.
void onTopLevelEntityCreated(ecs::Entity entity);

// This runs when an entity is created as a child of another entity. It adds a Transform that matches its parent.
void onChildEntityCreated(ecs::Entity child, ecs::Entity parent);

// This runs when an existing entity has its parent set manually. It updates its Transform to match its parent.
void onEntityAdopted(ecs::Entity child, ecs::Entity parent);

}  // namespace whal

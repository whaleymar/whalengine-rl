#include "Relationships.h"

#include "Components/Transform.h"
#include "whalECS/src/ECS.h"

namespace whal {

Attach::Attach(ecs::Entity target_, Vector2i offset_, DirectionParam directionParam_)
    : targetEntityID(target_.id()), offset(offset_), directionParam(directionParam_) {}

void Attach::initTarget(ecs::Entity self) {
    // adds self as child of target
    ecs::Entity targetEntity(targetEntityID);
    if (targetEntity.has<Children>()) {
        targetEntity.get<Children>().add(self);
    } else {
        targetEntity.add(Children({self.id()}));
    }
}

Orbit::Orbit(ecs::Entity target, s32 radius_, f32 rotationsPerSecond_, Vector2i targetOffset_)
    : targetID(target.id()), radius(radius_), rotationsPerSecond(rotationsPerSecond_), targetOffset(targetOffset_) {}

// adds self as child of target
void Orbit::initTarget(ecs::Entity self) {
    isTargetInitialized = true;
    ecs::Entity targetEntity(targetID);
    if (targetEntity.has<Children>()) {
        targetEntity.get<Children>().add(self);
    } else {
        targetEntity.add(Children({self.id()}));
    }

    // initialize current angle
    const Vector2i delta = self.get<Transform>().position - targetEntity.get<Transform>().position;
    currentAngle = delta.isZero() ? 0.0f : delta.as<f32>().angle();
}

Follow::Follow(ecs::Entity target_) : targetEntityID(target_.id()) {}

void Follow::initTarget(ecs::Entity self) {
    isTargetInitialized = true;
    ecs::Entity targetEntity(targetEntityID);
    currentTarget = targetEntity.get<Transform>().position;
}

void Children::add(ecs::Entity entity) {
    if (whal_find(entityIDs.begin(), entityIDs.end(), entity) != entityIDs.end()) {
        return;
    }
    entityIDs.push_back(entity.id());
}

}  // namespace whal

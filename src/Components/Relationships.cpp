#include "Relationships.h"

#include "Components/Draw.h"
#include "Components/Transform.h"
#include "whalECS/src/ECS.h"

namespace whal {

Attach::Attach(ecs::Entity target_, Vector2i offset_, DirectionParam directionParam_)
    : targetEntityID(target_.id()), offsetTexels(offset_), directionParam(directionParam_) {}

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
    const Vector2i delta = self.get<Transform2D>().position - targetEntity.get<Transform2D>().position;
    currentAngle = delta.isZero() ? 0.0f : getAngle(delta.as<f32>());
}

Follow::Follow(ecs::Entity target_) : targetEntityID(target_.id()) {}

void Follow::initTarget(ecs::Entity self) {
    isTargetInitialized = true;
    ecs::Entity targetEntity(targetEntityID);
    currentTarget = targetEntity.get<Transform2D>().position;

    // this is kind of hacky. Definitely shouldn't be adding children dynamically like this for game logic
    // #ifndef NDEBUG
    //     if (self.has<Children>()) {
    //         self.remove<Children>();
    //     }
    //     self.add<Children>();
    //     auto eOpt = ecs::World::getInstance().entity();
    //     if (eOpt.isExpected()) {
    //         auto debugTargetTracker = eOpt.value();
    //         debugTargetTracker.add<Transform2D>();
    //         debugTargetTracker.add(DrawDebug(Colors::Emerald));
    //         debugTargetTracker.add(Name("TargetTracker"));
    //         debugTargetTrackerID = debugTargetTracker.id();
    //         self.get<Children>().add(debugTargetTracker);
    //     }
    //
    //     auto eOpt2 = ecs::World::getInstance().entity();
    //     if (eOpt2.isExpected()) {
    //         auto debugPositionTracker = eOpt2.value();
    //         debugPositionTracker.add<Transform2D>();
    //         debugPositionTracker.add(DrawDebug(Colors::Magenta));
    //         debugPositionTracker.add(Name("PositionTracker"));
    //         debugPositionTrackerID = debugPositionTracker.id();
    //         self.get<Children>().add(debugPositionTracker);
    //     }
    // #endif
}

void Children::add(ecs::Entity entity) {
    if (whal_find(entityIDs.begin(), entityIDs.end(), entity) != entityIDs.end()) {
        // print("skipping duplicate add");
        // if (entity.has<Name>()) {
        //     print("\tduplicate was", entity.get<Name>());
        // }
        return;
    }
    // if (entity.has<Name>()) {
    //     print("adding", entity.get<Name>(), "to child list");
    // }
    entityIDs.push_back(entity.id());
    // print("num children: ", entities.size());
}

}  // namespace whal

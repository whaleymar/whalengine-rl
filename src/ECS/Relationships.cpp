#include "Relationships.h"

#include "ECS/Draw.h"
#include "ECS/Name.h"
#include "ECS/Transform.h"
#include "whalECS/src/ECS.h"

namespace whal {

Attach::Attach(ecs::Entity target_, Vector2i offset_) : targetEntityID(target_.id()), offsetTexels(offset_) {}

void Attach::initTarget(ecs::Entity self) {
    ecs::Entity targetEntity(targetEntityID);
    if (targetEntity.has<Children>()) {
        targetEntity.get<Children>().add(self);
    } else {
        targetEntity.add(Children({self.id()}));
    }
}

Follow::Follow(ecs::Entity target_) : targetEntityID(target_.id()) {}

void Follow::initTarget(ecs::Entity self) {
    isTargetInitialized = true;
    ecs::Entity targetEntity(targetEntityID);
    currentTarget = targetEntity.get<Transform2D>().position;

// this is kind of hacky. Definitely shouldn't be adding children dynamically like this for game logic
#ifndef NDEBUG
    if (self.has<Children>()) {
        self.remove<Children>();
    }
    self.add<Children>();
    auto eOpt = ecs::ECS::getInstance().entity();
    if (eOpt.isExpected()) {
        auto debugTargetTracker = eOpt.value();
        debugTargetTracker.add<Transform2D>();
        debugTargetTracker.add(DrawDebug(Colors::Emerald));
        debugTargetTracker.add(Name("TargetTracker"));
        debugTargetTrackerID = debugTargetTracker.id();
        self.get<Children>().add(debugTargetTracker);
    }

    auto eOpt2 = ecs::ECS::getInstance().entity();
    if (eOpt2.isExpected()) {
        auto debugPositionTracker = eOpt2.value();
        debugPositionTracker.add<Transform2D>();
        debugPositionTracker.add(DrawDebug(Colors::Magenta));
        debugPositionTracker.add(Name("PositionTracker"));
        debugPositionTrackerID = debugPositionTracker.id();
        self.get<Children>().add(debugPositionTracker);
    }
#endif
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

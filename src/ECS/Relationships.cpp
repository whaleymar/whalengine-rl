#include "Relationships.h"

// #include "ECS/Components/Draw.h"
// #include "ECS/Components/Name.h"
#include "ECS/Components/Transform.h"
// #include "Gfx/Color.h"

namespace whal {

Follow::Follow(ecs::Entity target_) : targetEntity(target_) {}

void Follow::initTarget(ecs::Entity self) {
    isTargetInitialized = true;
    currentTarget = targetEntity.get<Transform>().position;

    // this is kind of hacky. Definitely shouldn't be adding children dynamically like this for game logic
    // #ifndef NDEBUG
    //     if (self.has<Children>()) {
    //         self.remove<Children>();
    //     }
    //     self.add<Children>();
    //     auto eOpt = ecs::ECS::getInstance().entity();
    //     if (eOpt.isExpected()) {
    //         debugTargetTracker = eOpt.value();
    //         debugTargetTracker.add<Transform>();
    //         debugTargetTracker.add(Draw(Color::EMERALD));
    //         debugTargetTracker.add(Name("TargetTracker"));
    //         self.get<Children>().add(debugTargetTracker);
    //     }
    //
    //     auto eOpt2 = ecs::ECS::getInstance().entity();
    //     if (eOpt2.isExpected()) {
    //         debugPositionTracker = eOpt2.value();
    //         debugPositionTracker.add<Transform>();
    //         debugPositionTracker.add(Draw(Color::MAGENTA));
    //         debugPositionTracker.add(Name("PositionTracker"));
    //         self.get<Children>().add(debugPositionTracker);
    //     }
    // #endif
}

void Children::add(ecs::Entity entity) {
    if (std::find(entities.begin(), entities.end(), entity) != entities.end()) {
        // print("skipping duplicate add");
        // if (entity.has<Name>()) {
        //     print("\tduplicate was", entity.get<Name>());
        // }
        return;
    }
    // if (entity.has<Name>()) {
    //     print("adding", entity.get<Name>(), "to child list");
    // }
    entities.push_back(entity);
    // print("num children: ", entities.size());
}

}  // namespace whal

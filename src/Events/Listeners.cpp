#include "Listeners.h"

#include "Components/Transform.h"
#include "IGame.h"
#include "Map/Level.h"
#include "Sys/System.h"

namespace whal {

// ECS callback
void emitEntityDeathEvent(ecs::Entity entity) {
    Event.emit<evt::Death>(entity);
}

void onTopLevelEntityCreated(ecs::Entity entity) {
    // make entity owned by current scene
    entity.add<Transform>();
    ecs::Entity sceneRoot = System::getGame().getScene().self;
    if (sceneRoot.isValid()) {
        sceneRoot.addChild(entity);
    }
}

void onChildEntityCreated(ecs::Entity child, ecs::Entity parent) {
    Transform trans;
    Transform& pTrans = parent.get<Transform>();
    trans.setParent(pTrans, child);
    child.add(trans);
}

void onEntityAdopted(ecs::Entity child, ecs::Entity parent) {
    child.get<Transform>().setParent(parent.get<Transform>(), child);
}

}  // namespace whal

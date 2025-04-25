#include "Listeners.h"

#include "Components/Transform.h"
#include "Events/Events.h"
#include "IGame.h"
#include "Map/Scene.h"
#include "Sys/System.h"

namespace whal {

// ECS callback
void emitEntityDestroyedEvent(ecs::Entity entity) {
    Event.emit<evt::EntityDestroyed>(entity);
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
    if (!parent.isValid()) {
        // child was orphaned
        ecs::Entity sceneRoot = System::getGame().getScene().self;
        if (sceneRoot.isValid()) {
            sceneRoot.addChild(child);
        }

    } else {
        // want to update child's local transform without affecting its global transform
        Transform& trans = child.get<Transform>();
        bool ignoreVals[] = {trans.isIgnoreParentTranslation, trans.isIgnoreParentRotation, trans.isIgnoreParentScale};
        trans.isIgnoreParentTranslation = true;
        trans.isIgnoreParentRotation = true;
        trans.isIgnoreParentScale = true;
        trans.setParent(parent.get<Transform>(), child);
        trans.isIgnoreParentTranslation = ignoreVals[0];
        trans.isIgnoreParentRotation = ignoreVals[1];
        trans.isIgnoreParentScale = ignoreVals[2];
    }
}

}  // namespace whal

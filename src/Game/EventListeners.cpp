#include "EventListeners.h"

#include "Game/Components/Respawn.h"
#include "Systems/System.h"
#include "whalECS/src/ECS.h"

#include "ECS/Draw.h"
#include "ECS/Entities/Player.h"

using namespace whal;

void startListeners() {
    System::eventMgr.registerListener(Event::DEATH_EVENT, Listeners::ENTITY_DEATH_LISTENER);
}

void killListeners() {
    System::eventMgr.stopListening(Event::DEATH_EVENT, Listeners::ENTITY_DEATH_LISTENER);
}

// ECS callback
void emitEntityDeathEvent(ecs::Entity entity) {
    System::eventMgr.triggerEvent(Event::DEATH_EVENT, entity);
}

void playGameOverSound() {
    System::audio.playClip(Sfx::GAMEOVER);
}

void onEntityDeath(ecs::Entity entity) {
    // if (entity.has<Name>()) {
    //     print("Killed entity: ", entity.get<Name>());
    // }
    auto respawnOpt = entity.tryGet<Respawn>();
    if (!respawnOpt) {
        return;
    }

    auto respawn = *respawnOpt.value();
    Sprite sprite;  // needs to be created in main thread bc OpenGL

    // clang-format off
    System::schedule.eventFlow()
        .add(respawn.onDeath)
        .addWait(respawn.waitTime)
        .add(respawn.respawnCallback, sprite)
        .add(respawn.onRespawn);
    // clang-format on
}

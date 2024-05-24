#include "EventListeners.h"

#include "Systems/System.h"
#include "whalECS/src/ECS.h"

#include "ECS/Draw.h"
#include "ECS/Entities/Player.h"
#include "ECS/Tags.h"
// #include "Systems/Audio.h"

namespace whal {

void startListeners() {
    System::eventMgr.registerListener(Event::DEATH_EVENT, Listeners::PLAYER_DEATH_LISTENER);
}

void killListeners() {
    System::eventMgr.stopListening(Event::DEATH_EVENT, Listeners::PLAYER_DEATH_LISTENER);
}

// ECS callback
void emitEntityDeathEvent(ecs::Entity entity) {
    System::eventMgr.triggerEvent(Event::DEATH_EVENT, entity);
}

void playGameOverSound() {
    System::audio.playClip(Sfx::GAMEOVER);
}

// more like onPlayerDeath
void onEntityDeath(ecs::Entity entity) {
    // if (entity.has<Name>()) {
    //     print("Killed entity: ", entity.get<Name>());
    // }
    if (!entity.has<Player>()) {
        return;
    }
    Sprite sprite;  // needs to be created in main thread bc OpenGL
    System::schedule.after(&respawnPlayer, 2, sprite);

    // System::schedule.after(&playGameOverSound, 1.5);
    System::audio.playClip(Sfx::DEATH);
    System::audio.setMusicVolume(0.2);  // RESEARCH should also apply low pass filter here
}

}  // namespace whal

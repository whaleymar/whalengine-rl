#include "EventListeners.h"

#include "Systems/System.h"
#include "whalECS/src/ECS.h"

#include "ECS/Draw.h"
#include "ECS/Entities/Player.h"
#include "ECS/Tags.h"

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

// TODO Respawn component + system + system has this listener
void onEntityDeath(ecs::Entity entity) {
    // if (entity.has<Name>()) {
    //     print("Killed entity: ", entity.get<Name>());
    // }
    if (!entity.has<Player>()) {
        return;
    }

    Sprite sprite;  // needs to be created in main thread bc OpenGL
    const f32 respawnTime = 2;

    System::schedule.eventFlow()
        .add([]() {
            System::audio.playClip(Sfx::DEATH);
            System::audio.setMusicVolume(0.75);
            System::audio.setFilterMusic(AudioPlayer::Filter::LowPass);
        })
        .addWait(respawnTime)
        .add(&respawnPlayer, sprite)
        .add([]() {
            System::audio.setMusicVolume(1);
            System::audio.setFilterMusic(AudioPlayer::Filter::None);
        });
}

}  // namespace whal

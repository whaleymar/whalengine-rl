#include "System.h"

#include "Events/Listeners.h"
#include "IGame.h"
#include "Tween.h"

#include "Gfx/ShaderManager.h"
#include "Util/Print.h"

namespace whal {

static IGame* S_PGAME = nullptr;

void System::setPaused(bool pause) {
    IsPaused = pause;
    if (pause) {
        time.setMultiplier(0.0);
        audio.pauseClips(true);
        world.pause();
        event.emit<evt::Pause>(true);
    } else {
        time.setMultiplier(1.0);
        audio.pauseClips(false);
        world.unpause();
        event.emit<evt::Pause>(false);
    }
}

void System::Update() {
    // Engine Update
    input.update();
    time.update();
    schedule.tick(dt());
    audio.update();
    TweenManager::instance().update();
    world.update();

    // Game update
    mUpdateFunction();
}

bool System::IsValid() {
    return S_PGAME != nullptr && mUpdateFunction != nullptr;
}

void System::resetManagers() {
    time.mFrame = 0;
    time.mTimeElapsed = 0.0f;
    time.mTimeMultiplier = 1.0f;

    schedule.clear();
    audio.stopAll();
    TweenManager::instance().clear();
    ShaderManager::instance().reloadShaders();
}

bool System::start() {
    assert(!IsStarted);
    IsStarted = true;

    ShaderManager::instance().loadShaders();
    input.loadMappings();
    world.setEntityDeathCallback(&emitEntityDeathEvent);
    schedule.start();
    if (auto err = audio.init(); err) {
        print(*err);
        return true;
    }
    if (!audio.isValid()) {
        print("Error initializing audio manager");
        return true;
    }

    return false;
}

void System::end() {
    schedule.end();
    schedule.await();
    ShaderManager::instance().unloadAll();
}

void System::restart(bool resetPlayers) {
    resetManagers();
    event.emit<evt::Restart>(resetPlayers);
}

IGame& System::getGame() {
    return *S_PGAME;
}

void System::setGame(IGame& game) {
    S_PGAME = &game;
}

void System::setGameUpdate(UpdateFunction updateFunc) {
    mUpdateFunction = updateFunc;
}

}  // namespace whal

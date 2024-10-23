#include "System.h"
#include "Tween.h"

#include "IGame.h"

namespace whal {

static IGame* S_PGAME = nullptr;

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

void System::restart(bool resetPlayers) {
    time.mFrame = 0;
    time.mTimeElapsed = 0.0f;
    time.mTimeMultiplier = 1.0f;

    schedule.clear();
    audio.stopAll();
    TweenManager::instance().clear();
    eventMgr.emit<RestartEvent>(resetPlayers);
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

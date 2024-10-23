#include "System.h"
#include "Tween.h"

namespace whal {

void System::Update() {
    input.update();
    time.update();
    schedule.tick(dt());
    audio.update();
    TweenManager::instance().update();
    world.update();
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

}  // namespace whal

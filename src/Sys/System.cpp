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

}  // namespace whal

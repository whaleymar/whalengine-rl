#pragma once

#include "Systems/Audio.h"
#include "Systems/Deltatime.h"
#include "Systems/Event.h"
#include "Systems/Frametracker.h"
#include "Systems/InputHandler.h"
#include "Systems/JobScheduler.h"
#include "Systems/Random.h"
#include "whalECS/src/ECS.h"

namespace whal {

struct System {
    inline static InputHandler input;
    inline static Deltatime dt;
    inline static RNG rng;
    inline static Frametracker frame;
    inline static EventManager eventMgr;
    inline static AudioPlayer audio;
    inline static JobScheduler schedule;
    inline static ecs::World* world = &ecs::World::getInstance();

    static void setPaused(bool pause) {
        if (pause) {
            dt.setMultiplier(0.0);
            audio.pauseClips(true);
        } else {
            dt.setMultiplier(1.0);
            audio.pauseClips(false);
        }
        IsPaused = pause;
    }

    static void togglePause() { setPaused(!IsPaused); }
    static bool isPaused() { return IsPaused; }
    static void quit() { IsQuit = true; }
    static bool isQuit() { return IsQuit; }

private:
    inline static bool IsPaused = false;
    inline static bool IsQuit = false;
};

}  // namespace whal

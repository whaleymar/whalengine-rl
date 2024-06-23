#pragma once

// #include "Systems/Audio.h"
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
    // inline static AudioPlayer audio;
    inline static JobScheduler schedule;
    inline static ecs::World* world = &ecs::World::getInstance();

    static void setPaused(bool pause) {
        IsPaused = pause;
        if (pause) {
            dt.setMultiplier(0.0);
            // audio.pauseClips(true);
            world->pause();
        } else {
            dt.setMultiplier(1.0);
            // audio.pauseClips(false);
            world->unpause();
        }
    }

    static void togglePause() { setPaused(!IsPaused); }
    static bool isPaused() { return IsPaused; }
    static void quit() { IsQuit = true; }
    static bool isQuit() { return IsQuit; }

private:
    inline static bool IsPaused = false;
    inline static bool IsQuit = false;
};

// An interface for ECS Systems, but it's here to avoid circlular imports.
template <typename E, bool RunOnPause, typename... T>
    requires(std::is_base_of<IEvent<T...>, E>::value)
class IListen {
public:
    virtual ~IListen() { System::eventMgr.stopListening<E, T...>(mListener); }
    virtual void onEvent(T...) = 0;

protected:
    IListen()
        : mListener([this](T... args) {
              if constexpr (RunOnPause) {
                  this->onEvent(args...);
              } else {
                  if (!System::isPaused())
                      this->onEvent(args...);
              }
          }) {
        System::eventMgr.registerListener<E>(mListener);
    }

private:
    EventListener<T...> mListener;
};

}  // namespace whal

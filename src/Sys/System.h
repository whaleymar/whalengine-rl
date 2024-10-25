#pragma once

#include "Audio.h"
#include "Event.h"
#include "InputHandler.h"
#include "JobScheduler.h"
#include "Random.h"
#include "Sys/Prefab.h"
#include "Time.h"
#include "whalECS/src/ECS.h"

namespace whal {

class IGame;
class Engine;

struct System {
    friend Engine;

    using UpdateFunction = void (*)();
    inline static InputHandler input;
    inline static Time time;
    inline static RNG rng;
    inline static EventManager eventMgr;
    inline static AudioPlayer audio;
    inline static JobScheduler schedule;
    inline static ecs::World& world = ecs::World::getInstance();
    inline static Prefab prefab;

    static void setPaused(bool pause) {
        IsPaused = pause;
        if (pause) {
            time.setMultiplier(0.0);
            audio.pauseClips(true);
            world.pause();
            eventMgr.emit<PauseEvent>(true);
        } else {
            time.setMultiplier(1.0);
            audio.pauseClips(false);
            world.unpause();
            eventMgr.emit<PauseEvent>(false);
        }
    }

    static void Update();
    static bool IsValid();

    static f32 dt() { return time.getDeltaTime(); }
    static void togglePause() { setPaused(!IsPaused); }
    static bool isPaused() { return IsPaused; }
    static void quit() { IsQuit = true; }
    static bool isQuit() { return IsQuit; }
    static void restart(bool resetPlayers);
    static IGame& getGame();
    static void setGameUpdate(UpdateFunction updateFunc);

private:
    static void setGame(IGame& game);
    static void resetManagers();

    inline static bool IsPaused = false;
    inline static bool IsQuit = false;
    inline static UpdateFunction mUpdateFunction = nullptr;
};

// An interface for ECS Systems, but it's here to avoid circlular imports.
template <typename E, bool RunOnPause, typename... T>
    requires(std::is_base_of<IEvent<T...>, E>::value)
class IListen {
public:
    virtual ~IListen() { System::eventMgr.stopListening<E, T...>(mListener); }
    virtual void onEvent(E, T...) = 0;

protected:
    IListen()
        : mListener([this](T... args) {
              if constexpr (RunOnPause) {
                  this->onEvent(E{}, args...);
              } else {
                  if (!System::isPaused())
                      this->onEvent(E{}, args...);
              }
          }) {
        System::eventMgr.registerListener<E>(mListener);
    }

private:
    EventListener<T...> mListener;
};

}  // namespace whal

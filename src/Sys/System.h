#pragma once

#include "Audio.h"
#include "Event.h"
#include "InputHandler.h"
#include "JobScheduler.h"
#include "Prefab.h"
#include "Random.h"
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
    inline static EventManager event;
    inline static AudioPlayer audio;
    inline static JobScheduler schedule;
    inline static ecs::World& world = ecs::World::getInstance();
    inline static Prefab prefab;

    // Updates the engine and game state. Should not be called manually
    static void Update();

    // Confirms that a game is loaded
    static bool IsValid();

    static void setPaused(bool pause);
    static f32 dt() { return time.getDeltaTime(); }
    static void togglePause() { setPaused(!IsPaused); }
    static bool isPaused() { return IsPaused; }
    static void quit() { IsQuit = true; }
    static bool isQuit() { return IsQuit; }

    // Restarts the state of all modules
    static void restart(bool resetPlayers);

    // Returns a reference to the loaded game
    static IGame& getGame();

    // Sets the main game update method, which will run once every frame
    static void setGameUpdate(UpdateFunction updateFunc);

private:
    // Initializes the modules.
    // Assumes raylib context is initialized.
    static bool start();
    static void end();
    static void setGame(IGame& game);
    static void resetManagers();

    inline static bool IsPaused = false;
    inline static bool IsQuit = false;
    inline static bool IsStarted = false;
    inline static UpdateFunction mUpdateFunction = nullptr;
};

// An interface for ECS Systems, but it's here to avoid circlular imports.
template <typename E, bool RunOnPause, typename... T>
    requires(std::is_base_of<IEvent<T...>, E>::value)
class IListen {
public:
    virtual ~IListen() { System::event.stopListening<E, T...>(mListener); }
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
        System::event.registerListener<E>(mListener);
    }

private:
    EventListener<T...> mListener;
};

}  // namespace whal

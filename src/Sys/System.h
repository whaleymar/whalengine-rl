#pragma once

#include "Audio.h"
#include "Event.h"
#include "InputHandler.h"
#include "JobScheduler.h"
#include "Prefab.h"
#include "Random.h"
#include "Sys/Cursor.h"
#include "Time.h"
#include "whalECS/src/ECS.h"

namespace whal {

class IGame;
class Engine;

extern TimeManager Time;
extern InputHandler Input;
extern RNGManager Rng;
extern EventManager Event;
extern AudioPlayer Audio;
extern JobScheduler Schedule;
extern ecs::World& World;
extern PrefabManager Prefab;
extern CursorManager Cursor;

struct System {
    friend Engine;
    using UpdateFunction = void (*)();

    // Updates the engine and game state. Should not be called manually
    static void Update();

    // Confirms that a game is loaded
    static bool IsValid();

    static void setPaused(bool pause);
    static void togglePause();
    static bool isPaused();
    static void quit();
    static bool isQuit();

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
};

// An interface for ECS Systems, but it's here to avoid circlular imports.
template <typename E, bool RunOnPause, typename... T>
    requires(std::is_base_of<IEvent<T...>, E>::value)
class IListen {
public:
    virtual ~IListen() { Event.stopListening<E, T...>(mListener); }
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
        Event.registerListener<E>(mListener);
    }

private:
    EventListener<T...> mListener;
};

}  // namespace whal

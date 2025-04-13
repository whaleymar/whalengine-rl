#pragma once

#include "Random.h"
#include "whalECS/src/ECS.h"

namespace whal {

class IGame;
class Engine;
class Renderer;
class TimeManager;
class AudioPlayer;
class CursorManager;
class PrefabManager;
class JobScheduler;
class InputHandler;
class EventManager;

extern TimeManager& Time;
extern InputHandler& Input;
extern RNGManager Rng;  // The "main" Random Number Generator, but game logic can use other instances.
extern EventManager& Event;
extern AudioPlayer& Audio;
extern JobScheduler& Schedule;
extern ecs::World& World;
extern PrefabManager& Prefab;
extern CursorManager& Cursor;
extern Renderer& Graphics;

struct System {
    friend Engine;
    using UpdateFunction = void (*)();

    // Updates the engine and game state. Should not be called manually
    static void Update();

    // Confirms that a game is loaded
    static bool IsValid();

    // Pauses the game and sends pause signals to ECS Systems and Pause Event Observers
    static void setPaused(bool pause);
    static void togglePause();
    static bool isPaused();

    // Only sets the internal pause flag. Doesn't emit events or affect any modules.
    // The main purpose is to disable Observers with RunOnPause==false
    static void setQuietPaused(bool pause);
    static bool isQuietPaused();

    // Pauses the game and engine. Emits a special evt::EnginePause signal
    static void setEnginePaused(bool pause);
    static bool isEnginePaused();

    static void quit();
    static bool isQuit();

    // Restarts the state of all modules
    static void restart(bool resetPlayers);

    // Returns a reference to the loaded game
    static IGame& getGame();

    // Sets the main game update method, which will run once every frame
    static void setGameUpdate(UpdateFunction updateFunc);

    static void hotReload();

    // PRIVATE
    // Initializes the modules.
    // Assumes raylib context is initialized.
    static bool start();
    static void end();
    static void setGame(IGame& game);
    static void resetManagers();
};

}  // namespace whal

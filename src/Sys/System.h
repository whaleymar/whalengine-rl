#pragma once

#include "Audio.h"
#include "Cursor.h"
#include "Event.h"
#include "InputHandler.h"
#include "JobScheduler.h"
#include "Prefab.h"
#include "Random.h"
#include "Renderer.h"
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
extern Renderer Graphics;

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

extern "C" {
bool _EngineStart();
void _EngineSetGame(whal::IGame& game);
bool _EngineIsValid();
void _EngineReset();
void _EngineSleep(float seconds);
bool _EngineIsQuit();
void _EngineUpdate();
void _EngineEnd();
bool _EngineIsHotReloadRequested();

s32 _EngineGetWindowWidth();
s32 _EngineGetWindowHeight();
const char* _EngineGetWindowTitle();
s32 _EngineGetTargetFPS();

#ifndef NDEBUG
bool _EngineIsEditorMode();
void _EngineSetEditorMode(bool);
bool _EngineIsEditorSuspend();
void _EngineSetEditorSuspend(bool);
#endif
}

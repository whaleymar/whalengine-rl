#include "Sys/System.h"

#include "Events/Listeners.h"
#include "IGame.h"

#include "Gfx/ShaderManager.h"
#include "Settings.h"
#include "Util/Print.h"
#include "raylib.h"

namespace whal {

static IGame* S_PGAME = nullptr;

// MODULES
InputHandler Input = InputHandler();
TimeManager Time = TimeManager();
RNGManager Rng = RNGManager();
EventManager Event = EventManager();
AudioPlayer Audio = AudioPlayer();
JobScheduler Schedule = JobScheduler();
ecs::World& World = ecs::World::getInstance();
PrefabManager Prefab = PrefabManager();
CursorManager Cursor = CursorManager();
Renderer Graphics = Renderer();

// VARIABLES
static bool S_IS_PAUSED = false;
static bool S_IS_ENGINE_PAUSED = false;
static bool S_IS_PAUSED_AND_ENGINE_PAUSED = false;
static bool S_IS_QUIT = false;
static bool S_IS_STARTED = false;
static System::UpdateFunction S_UPDATE_FUNCTION = nullptr;
static bool S_DO_HOTRELOAD = false;
static f32 S_PREV_TIME_MULT;

void System::setPaused(bool pause) {
    S_IS_PAUSED = pause;
    if (pause) {
        S_PREV_TIME_MULT = Time.getMultiplier();
        Time.setMultiplier(0.0);
        Audio.pauseClips(true);
        World.pause();
        Event.emit<evt::Pause>(true);
    } else {
        Time.setMultiplier(S_PREV_TIME_MULT);
        Audio.pauseClips(false);
        World.unpause();
        Event.emit<evt::Pause>(false);
    }
}

void System::setQuietPaused(bool pause) {
    S_IS_PAUSED = pause;
}

bool System::isQuietPaused() {
    return S_IS_PAUSED;
}

void System::setEnginePaused(bool pause) {
    // handle special case where engine and game are both paused
    if (pause && S_IS_PAUSED) {
        S_IS_PAUSED_AND_ENGINE_PAUSED = true;
        S_IS_ENGINE_PAUSED = true;
        Event.emit<evt::EnginePause>(true);
        return;
    } else if (!pause && S_IS_PAUSED_AND_ENGINE_PAUSED) {
        S_IS_PAUSED_AND_ENGINE_PAUSED = false;
        S_IS_ENGINE_PAUSED = false;
        Event.emit<evt::EnginePause>(false);
        return;
    }

    S_IS_PAUSED = pause;
    S_IS_ENGINE_PAUSED = pause;
    if (pause) {
        S_PREV_TIME_MULT = Time.getMultiplier();
        Time.setMultiplier(0.0);
        Audio.pauseClips(true);
        Event.emit<evt::EnginePause>(true);
    } else {
        Time.setMultiplier(S_PREV_TIME_MULT);
        Audio.pauseClips(false);
        Event.emit<evt::EnginePause>(false);
    }
}

bool System::isEnginePaused() {
    return S_IS_ENGINE_PAUSED;
}

void System::togglePause() {
    setPaused(!S_IS_PAUSED);
}

bool System::isPaused() {
    return S_IS_PAUSED;
}

void System::quit() {
    S_IS_QUIT = true;
}

bool System::isQuit() {
    return S_IS_QUIT;
}

void System::Update() {
    // Engine Update
    Input.update();
    Time.update();
    Schedule.tick(Time.dt());
    Audio.update();
    World.update();

    // Game update
    S_UPDATE_FUNCTION();

    // Renderer update (want to make sure we're not clearing RenderTextures between world and game update, so do this at the very end)
    Graphics.update();
}

bool System::IsValid() {
    return S_PGAME != nullptr && S_UPDATE_FUNCTION != nullptr;
}

void System::resetManagers() {
    Time.mFrame = 0;
    Time.mTimeElapsed = 0.0f;
    Time.mTimeMultiplier = 1.0f;

    Schedule.clear();
    Audio.stopAll();
    ShaderManager::instance().reloadShaders();
}

bool System::start() {
    assert(!S_IS_STARTED);
    S_IS_STARTED = true;

    Graphics.init();
    ShaderManager::instance().loadShaders();
    World.setEntityDeathCallback(&emitEntityDeathEvent);
    World.setEntityCreateCallback(&onTopLevelEntityCreated);
    World.setEntityChildCreateCallback(&onChildEntityCreated);
    World.setEntityAdoptCallback(&onEntityAdopted);
    Schedule.start();
    if (auto err = Audio.init(); err) {
        print(*err);
        return true;
    }
    if (!Audio.isValid()) {
        print("Error initializing audio manager");
        return true;
    }

    return false;
}

void System::end() {
    Schedule.end();
    Schedule.await();
    Audio.end();
    ShaderManager::instance().unloadAll();
    World.clear();
    Graphics.end();
    S_IS_STARTED = false;
}

void System::restart(bool resetPlayers) {
    resetManagers();
    Event.emit<evt::Restart>(resetPlayers);
}

IGame& System::getGame() {
    return *S_PGAME;
}

void System::setGame(IGame& game) {
    S_PGAME = &game;
}

void System::setGameUpdate(UpdateFunction updateFunc) {
    S_UPDATE_FUNCTION = updateFunc;
}

void System::hotReload() {
    S_DO_HOTRELOAD = true;
}

}  // namespace whal

bool _EngineStart() {
    return whal::System::start();
}

void _EngineSetGame(whal::IGame& game) {
    whal::System::setGame(game);
}

bool _EngineIsValid() {
    return whal::System::IsValid();
}

void _EngineReset() {
    whal::System::resetManagers();
}

void _EngineSleep(float seconds) {
    whal::Time.sleep(seconds);
}

bool _EngineIsQuit() {
    return whal::System::isQuit();
}

void _EngineUpdate() {
    whal::System::Update();
}

void _EngineEnd() {
    whal::System::end();
}

bool _EngineIsHotReloadRequested() {
    if (whal::S_DO_HOTRELOAD) {
        whal::S_DO_HOTRELOAD = false;
        return true;
    }
    return false;
}

s32 _EngineGetWindowWidth() {
    return WINDOW_WIDTH_OS;
}

s32 _EngineGetWindowHeight() {
    return WINDOW_HEIGHT_OS;
}

const char* _EngineGetWindowTitle() {
    return WINDOW_TITLE;
}

s32 _EngineGetTargetFPS() {
    return FPS_TARGET;
}

#ifndef NDEBUG
bool _EngineIsEditorMode() {
    return EDITOR_MODE;
}

void _EngineSetEditorMode(bool isOn) {
    EDITOR_MODE = isOn;
    if (EDITOR_MODE) {
        rl::ShowCursor();
        // window resizing will be handled the next time imgui draws the UI
    } else {
        rl::HideCursor();

        // change dock size to match the OS screen
        const Vector2i osSize(WINDOW_WIDTH_OS, WINDOW_HEIGHT_OS);
        WINDOW_WIDTH_DOCK = osSize.x;
        WINDOW_HEIGHT_DOCK = osSize.y;

        // reset render size to (try to) match OS screen
        whal::Graphics.updateWindowSizes(osSize, osSize);

        if (EDITOR_SUSPEND) {
            _EngineSetEditorSuspend(false);
        }
    }
}

bool _EngineIsEditorSuspend() {
    return EDITOR_SUSPEND;
}

void _EngineSetEditorSuspend(bool isOn) {
    EDITOR_SUSPEND = isOn;
    if (EDITOR_SUSPEND) {
        whal::System::setEnginePaused(true);
    } else {
        whal::System::setEnginePaused(false);
    }
}
#endif

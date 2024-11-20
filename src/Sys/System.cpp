#include "Sys/System.h"

#include "Events/Listeners.h"
#include "IGame.h"

#include "Gfx/ShaderManager.h"
#include "Util/Print.h"

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
static bool S_IS_QUIT = false;
static bool S_IS_STARTED = false;
static System::UpdateFunction S_UPDATE_FUNCTION = nullptr;

void System::setPaused(bool pause) {
    S_IS_PAUSED = pause;
    if (pause) {
        Time.setMultiplier(0.0);
        Audio.pauseClips(true);
        World.pause();
        Event.emit<evt::Pause>(true);
    } else {
        Time.setMultiplier(1.0);
        Audio.pauseClips(false);
        World.unpause();
        Event.emit<evt::Pause>(false);
    }
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
    Input.loadMappings();
    World.setEntityDeathCallback(&emitEntityDeathEvent);
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
    ShaderManager::instance().unloadAll();
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

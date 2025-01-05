#pragma once

#include <raylib.h>
#include "IGame.h"
#include "Settings.h"
#include "Util/Print.h"

#include "Sys/System.h"

// WEB BUILD STUFF
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif
// /WEB

namespace whal {

namespace evt {
class Restart;
}

#if defined(DYNLIB)
#include <cstdlib>
#if defined(_WIN32)
#include <windows.h>
const char* DL_PATH = "build/libgamed.dll";
#else
#include <dlfcn.h>
const char* DL_PATH = "build/libgamed.so";
#endif
#else
extern "C" whal::IGame* CreateGame();
extern "C" void DestroyGame(whal::IGame* game);
#endif

class GameHandler {
public:
    using GameCreator = whal::IGame* (*)();
    using GameDestructor = void (*)(whal::IGame*);
    using IntGetter = s32 (*)();
    using Callback = void (*)();
    using GameSetter = void (*)(whal::IGame&);
    using BoolCB = bool (*)();
    using FloatFunc = void (*)(float);
    using StrGetter = const char* (*)();
    using BoolSetter = void (*)(bool);

    GameHandler() = default;
    bool isValid() const { return mLibHandle != nullptr; }

#if defined(DYNLIB)
    template <typename T>
    T getSymbol(const char* symbol) {
#ifdef _WIN32
        T fPtr = (T)GetProcAddress((HMODULE)mLibHandle, symbol);
#else
        T fPtr = (T)dlsym(mLibHandle, symbol);
#endif
        if (!fPtr) {
            print("Couldn't find", symbol, "symbol in", DL_PATH);
            mAllLoadsSuccessful = false;
            return nullptr;
        }
        return fPtr;
    }

    void getHandle() {
#ifdef _WIN32
        mLibHandle = LoadLibrary(DL_PATH);
#else
        mLibHandle = dlopen(DL_PATH, RTLD_NOW);
        const char* error = dlerror();
        if (error) {
            print("Error loading shared lib: ", error);
            return;
        }
#endif
    }

    void closeHandle() {
        if (!mLibHandle) {
            return;
        }
#ifdef _WIN32
        FreeLibrary((HMODULE)mLibHandle);
#else
        dlclose(mLibHandle);
        const char* error = dlerror();
        if (error) {
            print("dlclose error: ", error);
            return;
        }
#endif
        mLibHandle = nullptr;
    }
#endif

    // returns true if error
    bool loadLib() {
#if defined(DYNLIB)
        // do hot reloading setup
        if (mLibHandle) {
            closeHandle();
        }

        getHandle();
        if (!mLibHandle) {
            print("Failed to load libgamed.so");
            return true;
        }

        mAllLoadsSuccessful = true;
        CreateGameCB = getSymbol<GameCreator>("CreateGame");
        DestroyGameCB = getSymbol<GameDestructor>("DestroyGame");
        EngineStart = getSymbol<BoolCB>("_EngineStart");
        EngineSetGame = getSymbol<GameSetter>("_EngineSetGame");
        EngineIsValid = getSymbol<BoolCB>("_EngineIsValid");
        EngineReset = getSymbol<Callback>("_EngineReset");
        EngineSleep = getSymbol<FloatFunc>("_EngineSleep");
        EngineIsQuit = getSymbol<BoolCB>("_EngineIsQuit");
        EngineUpdate = getSymbol<Callback>("_EngineUpdate");
        EngineEnd = getSymbol<Callback>("_EngineEnd");
        EngineIsHotReload = getSymbol<BoolCB>("_EngineIsHotReloadRequested");
        GetWindowWidth = getSymbol<IntGetter>("WhalGetRenderWidth");
        GetWindowHeight = getSymbol<IntGetter>("WhalGetRenderHeight");
        GetTargetFPS = getSymbol<IntGetter>("WhalGetTargetFPS");
        GetWindowTitle = getSymbol<StrGetter>("WhalGetWindowTitle");
#ifndef NDEBUG
        GetEditorMode = getSymbol<BoolCB>("WhalIsEditorMode");
        SetEditorMode = getSymbol<BoolSetter>("WhalSetEditorMode");
#endif

        if (mAllLoadsSuccessful) {
            print("Loaded library successfully");
            return false;
        } else {
            print("Exiting...");
            closeHandle();
            return true;
        }
#else
        // We're linking statically, can address the callbacks directly
        CreateGameCB = CreateGame;
        DestroyGameCB = DestroyGame;
        EngineStart = _EngineStart;
        EngineSetGame = _EngineSetGame;
        EngineIsValid = _EngineIsValid;
        EngineReset = _EngineReset;
        EngineSleep = _EngineSleep;
        EngineIsQuit = _EngineIsQuit;
        EngineUpdate = _EngineUpdate;
        EngineEnd = _EngineEnd;
        EngineIsHotReload = _EngineIsHotReloadRequested;
        GetWindowWidth = WhalGetRenderWidth;
        GetWindowHeight = WhalGetRenderHeight;
        GetTargetFPS = WhalGetTargetFPS;
        GetWindowTitle = WhalGetWindowTitle;
#ifndef NDEBUG
        GetEditorMode = WhalIsEditorMode;
        SetEditorMode = WhalSetEditorMode;
#endif
        return false;

#endif
    }

#if defined(DYNLIB)
    void unloadLib() {
        if (!mLibHandle) {
            return;
        }

        if (mLibHandle) {
            closeHandle();
        }
    }
#endif

    BoolCB EngineStart;
    GameSetter EngineSetGame;
    BoolCB EngineIsValid;
    Callback EngineReset;
    FloatFunc EngineSleep;
    BoolCB EngineIsQuit;
    Callback EngineUpdate;
    Callback EngineEnd;
    IntGetter GetWindowWidth;
    IntGetter GetWindowHeight;
    IntGetter GetTargetFPS;
    StrGetter GetWindowTitle;
    GameCreator CreateGameCB;
    GameDestructor DestroyGameCB;
#ifndef NDEBUG
    BoolCB GetEditorMode;
    BoolSetter SetEditorMode;
#endif
    BoolCB EngineIsHotReload;

private:
    void* mLibHandle = nullptr;
    bool mAllLoadsSuccessful;
};

class Engine {
public:
    bool start() {
        // Load library
        bool err = mGameHandler.loadLib();
        if (err) {
            return true;
        }

        // Raylib initialization
        rl::SetTraceLogLevel(rl::LOG_WARNING);
        rl::InitWindow(mGameHandler.GetWindowWidth(), mGameHandler.GetWindowHeight(), mGameHandler.GetWindowTitle());
        rl::SetExitKey(rl::KEY_NULL);  // Escape quits by default

        rl::SetTargetFPS(mGameHandler.GetTargetFPS());

        // Set application icon for desktop builds
#ifndef __EMSCRIPTEN__
        if (rl::FileExists(ICON_IMAGE_PATH)) {
            mIconImage = rl::LoadImage(ICON_IMAGE_PATH);
            SetWindowIcon(mIconImage);
        }
#endif

        // Init modules
        return mGameHandler.EngineStart();
    }

    bool loadGame() {
        mGame = mGameHandler.CreateGameCB();
        if (!mGame) {
            return true;
        }

        mGameHandler.EngineSetGame(*mGame);
        if (mGame->start()) {
            print("Error initializing game");
            unloadGame(false);
            mGameHandler.EngineEnd();
            return true;
        }

        if (!mGameHandler.EngineIsValid()) {
            print("Game initialization is not valid. Make sure you registered an update function with System::setGameUpdate()");
            unloadGame(false);
            mGameHandler.EngineEnd();
            return true;
        }

        return false;
    }

    void unloadGame(bool nicely = true) {
        if (nicely) {
            mGame->end();
        }
        mGameHandler.DestroyGameCB(mGame);
        mGame = nullptr;
        mGameHandler.EngineReset();
    }

    void mainloop() {
#ifdef __EMSCRIPTEN__
        EM_ASM(FS.mkdir('/work'); FS.mount(IDBFS, {}, '/work'); FS.syncfs(true, function(err) { assert(!err); }););
        mGameHandler.EngineSleep(1);
        emscripten_set_main_loop(mGameHandler.EngineUpdate, 0,
                                 1);  // arg1: tells browser to control FPS. arg2: tells browser to simulate infinite loop for us
#else
        while (!rl::WindowShouldClose() && !mGameHandler.EngineIsQuit()) {
            mGameHandler.EngineUpdate();
            // hot reloading
#if defined(DYNLIB)
            if (mGameHandler.EngineIsHotReload()) {
// maintain previous editor state
#ifndef NDEBUG
                const bool isEditorMode = mGameHandler.GetEditorMode();
#endif

                unloadGame();
                mGameHandler.EngineEnd();
                print("Recompiling", DL_PATH);
                int result = std::system("make");
                if (result != 0) {
                    print("Error recompiling. Got code: ", result);
                    print("Reloading with old library");
                }
                print("Reloading Game library");
                bool err = mGameHandler.loadLib();
                if (err) {
                    print("Failed to reload library");
                    return;
                }
                print("Reloaded Game library");
                err = mGameHandler.EngineStart();
                if (err) {
                    print("Error Restarting Engine Modules");
                    return;
                }
                bool isError = loadGame();
                if (isError) {
                    break;
                }
                print("Loaded Game");

#ifndef NDEBUG
                mGameHandler.SetEditorMode(isEditorMode);
#endif
            }
#endif
        }
#endif
    }

    void end() {
        // Delete modules
        mGameHandler.EngineEnd();

        // Raylib end
#ifndef __EMSCRIPTEN__
        if (rl::FileExists(ICON_IMAGE_PATH)) {
            UnloadImage(mIconImage);
        }
#endif
        rl::CloseWindow();
    }

private:
    rl::Image mIconImage;
    GameHandler mGameHandler;
    IGame* mGame;
};

}  // namespace whal

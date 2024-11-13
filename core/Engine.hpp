#pragma once

#include <dlfcn.h>
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
#if defined(_WIN32)
const char* DL_PATH = "build/libgamed.dll";
#else
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

    GameHandler() = default;
    bool isValid() const { return mLibHandle != nullptr; }

    // TODO windows
#if defined(DYNLIB)
    template <typename T>
    T getSymbol(const char* symbol) {
        T fPtr = (T)dlsym(mLibHandle, symbol);
        if (!fPtr) {
            print("Couldn't find", symbol, "symbol in", DL_PATH);
            mAllLoadsSuccessful = false;
            return nullptr;
        }
        return fPtr;
    }

    // TODO windows
    void getHandle() { mLibHandle = dlopen(DL_PATH, RTLD_NOW); }

    // TODO windows
    void closeHandle() {
        dlclose(mLibHandle);
        const char* error = dlerror();
        if (error) {
            print("dlclose error: ", error);
            return;
        }
        mLibHandle = nullptr;
    }
#endif

    // returns true if error
    bool loadLib() {
#if defined(DYNLIB)
        // linux build, do hot reloading setup
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
        GetWindowWidth = getSymbol<IntGetter>("GetRenderWidth");
        GetWindowHeight = getSymbol<IntGetter>("GetRenderHeight");

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
        GetWindowWidth = GetRenderWidth;
        GetWindowHeight = GetRenderHeight;
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
    GameCreator CreateGameCB;
    GameDestructor DestroyGameCB;

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
        SetTraceLogLevel(LOG_WARNING);
        InitWindow(mGameHandler.GetWindowWidth(), mGameHandler.GetWindowHeight(), WINDOW_TITLE);
        SetExitKey(KEY_NULL);  // Escape quits by default

        SetTargetFPS(FPS_TARGET);
        // SetTargetFPS(144); // for testing

        // Set application icon for desktop builds
#ifndef __EMSCRIPTEN__
        if (FileExists(ICON_IMAGE_PATH)) {
            mIconImage = LoadImage(ICON_IMAGE_PATH);
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
            return true;
        }

        if (!mGameHandler.EngineIsValid()) {
            print("Game initialization is not valid. Make sure you registered an update function with System::setGameUpdate()");
            return true;
        }
        // System::event.emit<evt::Restart>();  // For some reason, map objects (not tiles) disappear unless I do this (only happens on restart, not
        //                                      // regular start). TODO it's definitely a bug

        return false;
    }

    void unloadGame() {
        mGame->end();
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
        while (!WindowShouldClose() && !mGameHandler.EngineIsQuit()) {
            mGameHandler.EngineUpdate();
            // hot reloading
#if defined(DYNLIB)
            if (IsKeyPressed(KEY_R)) {
                unloadGame();
                print("Reloading Game library");
                mGameHandler.EngineEnd();
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
                loadGame();
                print("Loaded Game");
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
        if (FileExists(ICON_IMAGE_PATH)) {
            UnloadImage(mIconImage);
        }
#endif
        CloseWindow();
    }

private:
    Image mIconImage;
    GameHandler mGameHandler;
    IGame* mGame;
};

}  // namespace whal

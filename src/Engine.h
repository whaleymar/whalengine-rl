#pragma once

#include <raylib.h>

#include "IGame.h"
#include "Settings.h"

#include "Events/Listeners.h"
#include "Gfx/ShaderManager.h"
#include "Sys/System.h"
#include "Util/Print.h"

// WEB BUILD STUFF
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif
// /WEB

#include "Game/Game.h"  // TEMP

namespace whal {

class Engine {
public:
    bool start() {
        // RAYLIB INITIALIZATION
        SetTraceLogLevel(LOG_WARNING);
        InitWindow(WINDOW_WIDTH_RENDER, WINDOW_HEIGHT_RENDER, WINDOW_TITLE);
        SetExitKey(KEY_NULL);  // Escape quits by default

        SetTargetFPS(FPS_TARGET);
        // SetTargetFPS(144); // for testing

#ifndef __EMSCRIPTEN__
        if (FileExists(ICON_IMAGE_PATH)) {
            mIconImage = LoadImage(ICON_IMAGE_PATH);
            SetWindowIcon(mIconImage);
        }
#endif

        // GRAPHICS INITIALIZATION
        ShaderManager::instance().loadShaders();

        // SYSTEM INITIALIZATION
        System::input.loadMappings();
        System::world.setEntityDeathCallback(&emitEntityDeathEvent);
        System::schedule.start();
        if (auto err = System::audio.init(); err) {
            print(*err);
            return true;
        }
        if (!System::audio.isValid()) {
            print("Error initializing audio manager");
            return true;
        }

        return false;
    }

    bool loadGame() {
        mGame = new Game();
        System::setGame(*mGame);
        if (mGame->start()) {
            print("Error initializing game");
            return true;
        }

        if (!System::IsValid()) {
            print("Game initialization is not valid. Make sure you registered an update function with System::setGameUpdate()");
            return true;
        }

        return false;
    }

    void unloadGame() {
        mGame->end();
        delete mGame;
        mGame = nullptr;
    }

    void mainloop() {
#ifdef __EMSCRIPTEN__
        EM_ASM(FS.mkdir('/work'); FS.mount(IDBFS, {}, '/work'); FS.syncfs(true, function(err) { assert(!err); }););
        System::time.sleep(1);
        emscripten_set_main_loop(System::Update, 0, 1);  // arg1: tells browser to control FPS. arg2: tells browser to simulate infinite loop for us
#else
        while (!WindowShouldClose() && !System::isQuit()) {
            System::Update();
        }
#endif
    }

    void end() {
        // SYSTEM END
        System::schedule.end();
        System::schedule.await();

        // GRAPHICS END
        ShaderManager::instance().unloadAll();

        // RAYLIB END
#ifndef __EMSCRIPTEN__
        if (FileExists(ICON_IMAGE_PATH)) {
            UnloadImage(mIconImage);
        }
#endif
        CloseWindow();
    }

private:
    Image mIconImage;
    IGame* mGame;
};

}  // namespace whal

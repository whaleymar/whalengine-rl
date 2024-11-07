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

// TEMP
// these will eventually be extern
#include "Game/Game.h"
whal::IGame* CreateGame() {
    return new Game();
}
void DestroyGame(whal::IGame* game) {
    delete game;
}

namespace whal {

namespace evt {
class Restart;
}

class Engine {
public:
    bool start() {
        // Raylib initialization
        SetTraceLogLevel(LOG_WARNING);
        InitWindow(WINDOW_WIDTH_RENDER, WINDOW_HEIGHT_RENDER, WINDOW_TITLE);
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
        return System::start();
    }

    bool loadGame() {
        mGame = CreateGame();
        System::setGame(*mGame);
        if (mGame->start()) {
            print("Error initializing game");
            return true;
        }

        if (!System::IsValid()) {
            print("Game initialization is not valid. Make sure you registered an update function with System::setGameUpdate()");
            return true;
        }
        System::event.emit<evt::Restart>();  // For some reason, map objects (not tiles) disappear unless I do this (only happens on restart, not
                                             // regular start)

        return false;
    }

    void unloadGame() {
        mGame->end();
        DestroyGame(mGame);
        mGame = nullptr;
        System::resetManagers();
        System::world.clear();
    }

    void mainloop() {
#ifdef __EMSCRIPTEN__
        EM_ASM(FS.mkdir('/work'); FS.mount(IDBFS, {}, '/work'); FS.syncfs(true, function(err) { assert(!err); }););
        System::time.sleep(1);
        emscripten_set_main_loop(System::Update, 0, 1);  // arg1: tells browser to control FPS. arg2: tells browser to simulate infinite loop for us
#else
        while (!WindowShouldClose() && !System::isQuit()) {
            System::Update();
            // for testing
            // if (IsKeyPressed(KEY_R)) {
            //     unloadGame();
            //     loadGame();
            // }
        }
#endif
    }

    void end() {
        // Delete modules
        System::end();

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
    IGame* mGame;
};

}  // namespace whal

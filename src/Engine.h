#pragma once

#include <raylib.h>
#include <type_traits>

#include "Components/Collision.h"
#include "IGame.h"
#include "Map/Level.h"
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

namespace whal {

template <class T>
    requires std::is_base_of_v<IGame, T> && Singleton<T> && StaticUpdate<T>
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

        // GLOBAL REFS INITIALIZATION
        Collision::registerGame(&T::instance());
        Map::registerGame(&T::instance());

        // GAME INITIALIZATION
        return T::instance().start();
    }

    void mainloop() {
#ifdef __EMSCRIPTEN__
        EM_ASM(FS.mkdir('/work'); FS.mount(IDBFS, {}, '/work'); FS.syncfs(true, function(err) { assert(!err); }););
        System::time.sleep(1);
        emscripten_set_main_loop(T::update, 0, 1);  // arg1: tells browser to control FPS. arg2: tells browser to simulate infinite loop for us
#else
        while (!WindowShouldClose() && !System::isQuit()) {
            T::update();
        }
#endif
    }

    void end() {
        // GAME END
        T::instance().end();

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
};

}  // namespace whal

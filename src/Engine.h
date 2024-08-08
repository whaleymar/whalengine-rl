#pragma once

#include <raylib.h>
#include <type_traits>
#include "IGame.h"
#include "Settings.h"

#include "Events/Listeners.h"
#include "Gfx/ShaderManager.h"
#include "Sys/System.h"
#include "Util/Print.h"

namespace whal {

template <typename T>
concept Singleton = requires {
    { T::instance() } -> std::same_as<T&>;  // Checks that T::instance() returns T&
};

template <class T>
    requires std::is_base_of_v<IGame, T> && Singleton<T>
class Engine {
public:
    bool start() {
        // RAYLIB INITIALIZATION
        SetTraceLogLevel(LOG_WARNING);
        InitWindow(WINDOW_WIDTH_ACTUAL, WINDOW_HEIGHT_ACTUAL, WINDOW_TITLE);
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

        // GAME INITIALIZATION
        return T::instance().start();
    }

    void mainloop() { T::instance().mainloop(); }

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

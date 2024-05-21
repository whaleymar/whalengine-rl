#pragma once

#include <optional>

#include "Util/Vector.h"
#include "whalECS/src/Expected.h"

namespace whal {

class Scene;

namespace ecs {
class Entity;
}

}  // namespace whal

class Game {
public:
    static Game& instance() {
        static Game instance_;
        return instance_;
    }

    Game(const Game& other) = delete;
    void operator=(const Game&) = delete;

    bool startup();
    void mainloop();
    void end();

    std::optional<Error> loadScene(const char* name);
    void unloadScene();
    std::optional<Error> reloadScene();
    whal::Scene& getScene();
    void updateLoadedLevels(whal::Vector2f cameraWorldPosPixels);
    void updateLevelCamera(bool overrideCache = false);

private:
    Game();
    // whal::Scene mActiveScene;
    // EventListener<ecs::Entity> mEntityDeathListener;
    bool mIsSceneLoaded;
};

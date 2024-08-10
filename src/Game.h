#pragma once

#include "CorradeOptional.h"

#include "Events/Events.h"
#include "IGame.h"
#include "Map/Level.h"
#include "Sys/System.h"
#include "Util/Vector.h"
#include "whalECS/src/Expected.h"

namespace whal {

struct Scene;

namespace ecs {
class Entity;
}

}  // namespace whal

class Game : public whal::IGame,
             public whal::IListen<whal::DeathEvent, true, whal::ecs::Entity>,
             public whal::IListen<whal::ShaderReloadEvent, true> {
public:
    static Game& instance() {
        static Game instance_;
        return instance_;
    }

    Game(const Game& other) = delete;
    void operator=(const Game&) = delete;

    bool start() override;
    void mainloop() override;
    void end() override;
    whal::Scene& getScene() override;

    // calls removeEntityFromLevel on killed entity
    void onEvent(whal::DeathEvent, whal::ecs::Entity) override;
    void onEvent(whal::ShaderReloadEvent) override;

    void removeEntityFromLevel(whal::ecs::Entity entity);
    Corrade::Containers::Optional<Error> loadScene(const char* name, bool resetPlayers);
    void unloadScene(bool resetPlayers);
    Corrade::Containers::Optional<Error> reloadScene(bool resetPlayers = false);
    void updateLoadedLevels(Vector2f cameraWorldPosPixels);
    void checkIfInNewLevel(bool overrideCache = false);

private:
    Game() = default;

    whal::Scene mActiveScene;
    bool mIsSceneLoaded = false;
};

#pragma once

#include "CorradeOptional.h"

#include "Events/Events.h"
#include "IGame.h"
#include "Map/Level.h"
#include "Sys/System.h"
#include "Util/Vector.h"
#include "whalECS/src/Expected.h"

typedef struct Font Font;
typedef struct Camera2D Camera2D;

namespace whal {

struct Scene;

namespace ecs {
class Entity;
}

}  // namespace whal

class Game : public whal::IGame, public whal::IListen<whal::DeathEvent, true, whal::ecs::Entity> {
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

    // calls removeEntityFromLevel on killed entity
    void onEvent(whal::DeathEvent, whal::ecs::Entity) override;

    void removeEntityFromLevel(whal::ecs::Entity entity);
    Corrade::Containers::Optional<Error> loadScene(const char* name, bool resetPlayers);
    void unloadScene(bool resetPlayers);
    Corrade::Containers::Optional<Error> reloadScene(bool resetPlayers = false);
    whal::Scene& getScene();
    void updateLoadedLevels(Vector2f cameraWorldPosPixels);
    void checkIfInNewLevel(bool overrideCache = false);
    void loadFont(const char* fontPath, s32 size, s32* codePoints, s32 codePointsCount);
    const Font* getFont() const;
    Camera2D* getWorldCamera() const { return mWorldSpaceCamera; }

private:
    Game();
    ~Game() override;

    whal::Scene mActiveScene;

    // I can't figure out to unique_ptr a forward declared type
    Font* mFont;
    Camera2D* mWorldSpaceCamera;
    Camera2D* mScreenSpaceCamera;
    bool mIsSceneLoaded = false;
};

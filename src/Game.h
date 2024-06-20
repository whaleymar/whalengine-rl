#pragma once

#include "CorradeOptional.h"

#include "Events/Events.h"
#include "Map/Level.h"
#include "Systems/System.h"
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

class Game : public whal::IListen<whal::DeathEvent, true, whal::ecs::Entity> {
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

    // calls removeEntityFromLevel on killed entity
    void onEvent(whal::ecs::Entity) override;

    void removeEntityFromLevel(whal::ecs::Entity entity);
    Corrade::Containers::Optional<Error> loadScene(const char* name);
    void unloadScene();
    Corrade::Containers::Optional<Error> reloadScene();
    whal::Scene& getScene();
    void updateLoadedLevels(Vector2f cameraWorldPosPixels);
    void updateLevelCamera(bool overrideCache = false);
    void loadFont(const char* fontPath, s32 size, s32* codePoints, s32 codePointsCount);
    const Font* getFont() const;
    Camera2D* getWorldCamera() const { return mWorldSpaceCamera; }

private:
    Game();
    ~Game();

    whal::Scene mActiveScene;

    // I can't figure out to unique_ptr a forward declared type
    Font* mFont;
    Camera2D* mWorldSpaceCamera;
    Camera2D* mScreenSpaceCamera;
    bool mIsSceneLoaded;
};

#pragma once

#include <optional>

#include "Map/Level.h"
#include "Systems/Event.h"
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
    void loadFont(const char* fontPath, s32 size, s32* codePoints, s32 codePointsCount);
    const Font* getFont() const;

private:
    Game();
    ~Game();

    whal::Scene mActiveScene;
    whal::EventListener<whal::ecs::Entity> mEntityDeathListener;
    Font* mFont;
    Camera2D* mWorldSpaceCamera;
    Camera2D* mScreenSpaceCamera;
    bool mIsSceneLoaded;
};

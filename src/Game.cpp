#include "Game.h"

#include <raylib.h>

#include "Gfx/Texture.h"
#include "Settings.h"
#include "Util/Print.h"
#include "Util/Types.h"

// GAME SETTINGS

constexpr f32 MAX_LOAD_DISTANCE_TEXELS = WINDOW_WIDTH_TEXELS * 3;

// /GAME SETTINGS

using namespace whal;

Game::Game() {}

bool Game::startup() {
    InitWindow(WINDOW_WIDTH_ACTUAL, WINDOW_HEIGHT_ACTUAL, WINDOW_TITLE);

    // do this before any font/texture stuff or the settings seem to get fucked
    Camera2D camera;
    camera.target = Vector2(0.0f, 0.0f);
    camera.offset = Vector2(WINDOW_WIDTH_ACTUAL / 2.0f, WINDOW_HEIGHT_ACTUAL / 2.0f);
    camera.zoom = 1.0f;

    std::optional<Error> err = TextureManager::instance().loadAndRegisterAtlas(SPRITE_TEXTURE_PATH, ATLAS_METADATA_PATH, TEXNAME_SPRITE);
    if (err) {
        print(err);
        return true;
    }

    SetTargetFPS(FPS_TARGET);

    // TODO audio
    // start event listeners
    // start scheduler
    // parse map project

    return false;
}

void Game::mainloop() {
    // TODO
}

void Game::end() {
    CloseWindow();
    // TODO stop scheduler
    // kill listeners
}

std::optional<Error> Game::loadScene(const char* filename) {
    // if (mIsSceneLoaded) {
    //     unloadScene();
    // }
    //
    // auto errOpt = parseWorld(filename, mActiveScene);
    // if (errOpt) {
    //     mIsSceneLoaded = false;
    //     return errOpt;
    // }
    // Vector2f cameraPos = toFloatVec(getCameraPosition());
    // updateLoadedLevels(cameraPos);
    // // TODO spawn player at start pos?
    //
    // mIsSceneLoaded = true;
    return std::nullopt;
}

void Game::unloadScene() {
    // while (!mActiveScene.loadedLevels.empty()) {
    //     auto& lvl = mActiveScene.loadedLevels.back();
    //     unloadLevel(lvl);
    //     mActiveScene.loadedLevels.pop_back();
    // }
    // std::set<ecs::Entity> toKill = std::move(mActiveScene.childEntities);
    // for (auto entity : toKill) {
    //     entity.kill();
    // }
    // mIsSceneLoaded = false;
}

std::optional<Error> Game::reloadScene() {
    // auto errOpt = loadScene(mActiveScene.name.c_str());
    // if (!errOpt) {
    //     updateLevelCamera(true);
    // }
    // return errOpt;

    return std::nullopt;
}

// Scene& Game::getScene() {
//     return mActiveScene;
// }

void Game::updateLoadedLevels(Vector2f cameraWorldPosPixels) {
    // Vector2f cameraWorldPosTexels = cameraWorldPosPixels * TEXELS_PER_PIXEL;
    // for (auto lvl : mActiveScene.allLevels) {
    //     Vector2f lvlCenterPos = lvl.worldPosOriginTexels - lvl.sizeTexels * Vector2f(-0.5, 0.5);
    //     const bool shouldLoad = (lvlCenterPos - cameraWorldPosTexels).len() <= MAX_LOAD_DISTANCE_TEXELS;
    //     auto it = std::find(mActiveScene.loadedLevels.begin(), mActiveScene.loadedLevels.end(), lvl);
    //     const bool isLevelLoaded = it != mActiveScene.loadedLevels.end();
    //
    //     if (shouldLoad && !isLevelLoaded) {
    //         std::optional<Error> errOpt = loadLevel(lvl);
    //         if (errOpt) {
    //             print(*errOpt);
    //             continue;
    //         }
    //     } else if (!shouldLoad && isLevelLoaded) {
    //         unloadAndRemoveLevel(*it);
    //     }
    // }
}

void Game::updateLevelCamera(bool overrideCache) {
    // if (PlayerSystem::instance()->getEntitiesRef().empty() || !mIsSceneLoaded || CameraSystem::instance()->getEntitiesRef().empty()) {
    //     return;
    // }
    // static std::string lastLevel = "default";
    // std::string curLevel;
    // ecs::Entity player = PlayerSystem::instance()->first();
    // ecs::Entity camera = CameraSystem::instance()->first();
    // Vector2f playerPosTexels = toFloatVec(player.get<Transform>().position) * TEXELS_PER_PIXEL;
    // auto levelOpt = mActiveScene.getLevelAt(playerPosTexels);
    //
    // bool doDefaultCamera = false;
    // if (!levelOpt) {
    //     // print("Not within any level boundaries");
    //     doDefaultCamera = true;
    // } else {
    //     curLevel = levelOpt.value().filepath;
    //     if (!overrideCache && curLevel == lastLevel) {
    //         return;
    //     }
    //     print("not skipping");
    //     lastLevel = curLevel;
    //     Expected<ActiveLevel*> activeOpt = mActiveScene.getLoadedLevel(levelOpt.value());
    //     if (!activeOpt.isExpected()) {
    //         print("Couldn't load level. Got error:", activeOpt.error());
    //         doDefaultCamera = true;
    //     } else {
    //         if (activeOpt.value()->cameraFollow) {
    //             Follow follow = activeOpt.value()->cameraFollow.value();
    //             follow.targetEntity = player;
    //             if (camera.has<Follow>()) {
    //                 camera.set(follow);
    //             } else {
    //                 camera.add(follow);
    //             }
    //             return;
    //         } else {
    //             Vector2i focalPoint = activeOpt.value()->cameraFocalPoint;
    //             if (camera.has<Follow>()) {
    //                 camera.remove<Follow>();
    //             }
    //             camera.add(createCameraMoveController(camera.get<Transform>().position, focalPoint));
    //             System::dt.setMultiplier(0.0);
    //             return;
    //         }
    //     }
    // }
    // if (doDefaultCamera) {
    //     curLevel = "default";
    //     if (!overrideCache && curLevel == lastLevel) {
    //         return;
    //     }
    //     lastLevel = curLevel;
    //     Follow follow = Follow(player);
    //     if (camera.has<Follow>()) {
    //         camera.set(follow);
    //     } else {
    //         camera.add(follow);
    //     }
    // }
}

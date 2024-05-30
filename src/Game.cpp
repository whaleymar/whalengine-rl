#include "Game.h"

#include <raylib.h>

#include "ECS/Blaster.h"
#include "ECS/Entities/Camera.h"
#include "ECS/Systems/Animation.h"
#include "ECS/Systems/CallbackSystem.h"
#include "ECS/Systems/CollisionManager.h"
#include "ECS/Systems/ControllerSystem.h"
#include "ECS/Systems/Gfx.h"
#include "ECS/Systems/Lifetime.h"
#include "ECS/Systems/Physics.h"
#include "ECS/Systems/Rails.h"
#include "ECS/Systems/RelationshipManager.h"
#include "ECS/Systems/TagTrackers.h"
#include "ECS/Systems/TriggerSystem.h"

#include "Game/DebugScene.h"
#include "Game/EventListeners.h"
#include "Gfx/Texture.h"
#include "Map/Level.h"
#include "Map/Tiled.h"
#include "Settings.h"
#include "Systems/PauseMenu.h"
#include "Systems/System.h"
#include "Util/Print.h"
#include "Util/Types.h"

// GAME SETTINGS

constexpr f32 MAX_LOAD_DISTANCE_TEXELS = WINDOW_WIDTH_TEXELS * 3;

// /GAME SETTINGS

using namespace whal;

Game::Game() : mEntityDeathListener(EventListener<ecs::Entity>(&removeEntityFromLevel)) {
    System::eventMgr.registerListener(Event::DEATH_EVENT, mEntityDeathListener);
    mWorldSpaceCamera = new Camera2D();
    mScreenSpaceCamera = new Camera2D();
    mFont = new Font();

    SetTraceLogLevel(LOG_WARNING);
}

Game::~Game() {
    delete mWorldSpaceCamera;
    delete mScreenSpaceCamera;
    delete mFont;
}

bool Game::startup() {
    InitWindow(WINDOW_WIDTH_ACTUAL, WINDOW_HEIGHT_ACTUAL, WINDOW_TITLE);

    // do this before any font/texture stuff or the settings seem to get fucked
    mWorldSpaceCamera->target = Vector2(0.0f, 0.0f);
    mWorldSpaceCamera->offset = Vector2(WINDOW_WIDTH_PIXELS / 2.0f, WINDOW_HEIGHT_PIXELS / 2.0f);  // center camera
    mWorldSpaceCamera->zoom = 1.0f;
    mWorldSpaceCamera->rotation = 0.0f;

    mScreenSpaceCamera->target = Vector2(0.0f, 0.0f);
    mScreenSpaceCamera->zoom = 1.0f;
    mScreenSpaceCamera->rotation = 0.0f;

    loadFont(FONT_PATH, 18, 0, 0);

    std::optional<Error> err = TextureManager::instance().loadAndRegisterAtlas(SPRITE_TEXTURE_PATH, ATLAS_METADATA_PATH, TEXNAME_SPRITE);
    if (err) {
        print(*err);
        return true;
    }

    SetTargetFPS(FPS_TARGET);

    if (!System::audio.isValid()) {
        print("Error initializing audio manager");
        return true;
    }
    System::ecs->setEntityDeathCallback(&emitEntityDeathEvent);
    System::schedule.start();
    startListeners();

    return false;
}

void Game::mainloop() {
    auto controlSystemRB = System::ecs->registerSystem<ControllerSystemRB>();
    auto controlSystemFree = System::ecs->registerSystem<ControllerSystemFree>();
    auto pathSystem = System::ecs->registerSystem<RailsSystem>();
    auto physicsSystem = System::ecs->registerSystem<PhysicsSystem>();
    auto spriteSystem = System::ecs->registerSystem<SpriteSystem>();
    auto drawSystem = System::ecs->registerSystem<DrawSystem>();
    auto drawDebugSystem = System::ecs->registerSystem<DrawDebugSystem>();
    auto animationSystem = System::ecs->registerSystem<AnimationSystem>();
    auto lifetimeSystem = System::ecs->registerSystem<LifetimeSystem>();
    System::ecs->registerSystem<MovableActorTracker>();  // dependency of TriggerSystem
    auto triggerSystem = System::ecs->registerSystem<TriggerSystem>();
    auto frameEndSystem = System::ecs->registerSystem<OnFrameEndSystem>();
    auto followSystem = System::ecs->registerSystem<FollowSystem>();
    auto attachSystem = System::ecs->registerSystem<AttachSystem>();

    // single-component systems for running psuedo-destructors / updating some global var
    auto actorsMgr = ActorsManager::instance();
    auto solidsMgr = SolidsManager::instance();
    auto semiSolidsMgr = SemiSolidsManager::instance();

    // these don't have update methods:
    auto playerMgr = PlayerSystem::instance();
    auto cameraMgr = CameraSystem::instance();
    auto childMgr = EntityChildSystem::instance();
    System::ecs->registerSystem<ProjectileSystem>();

    // load scene // TODO separate function
    auto err = loadTestMap();
    if (err) {
        print("Error loading debug scene: ", err.value());
        return;
    }

    System::audio.playMusic("data/audio/music/provingGroundsTheme.mp3");

    actorsMgr->update();
    solidsMgr->update();
    semiSolidsMgr->update();

    RenderTexture2D targetTexture = LoadRenderTexture(WINDOW_WIDTH_PIXELS, WINDOW_HEIGHT_PIXELS);  // where we'll draw objects to
    // Color clearColor = {51, 76, 76, 255};
    Color clearColor = {5, 5, 5, 255};

    // flip y axis bc openGL
    Rectangle screenSourceRec = {0.0f, 0.0f, static_cast<f32>(targetTexture.texture.width), -1 * static_cast<f32>(targetTexture.texture.height)};
    Rectangle screenDestRec = {-VIRTUAL_SCREEN_RATIO, -VIRTUAL_SCREEN_RATIO, WINDOW_WIDTH_ACTUAL + (VIRTUAL_SCREEN_RATIO * 2),
                               WINDOW_HEIGHT_ACTUAL + (VIRTUAL_SCREEN_RATIO * 2)};
    while (!WindowShouldClose() && !System::isQuit()) {
        System::input.update();
        if (System::frame.getFrame() == 0) {
            Vector2f cameraPos = toFloatVec(getCameraPosition());
            updateLoadedLevels(cameraPos);
        }

        System::dt.update();
        System::schedule.tick(System::dt());
        System::frame.update();
        System::audio.update();

        controlSystemRB->update();
        controlSystemFree->update();
        pathSystem->update();
        physicsSystem->update();

        // relationships should run after physics
        attachSystem->update();
        followSystem->update();

        triggerSystem->update();

        lifetimeSystem->update();

        // Update Scene
        updateLevelCamera();

        // Only rendering remains, so we can do "end of frame" stuff now
        frameEndSystem->update();
        System::ecs->killEntities();
        actorsMgr->update();
        solidsMgr->update();
        semiSolidsMgr->update();

        animationSystem->update();

#ifndef NDEBUG
        // if (IsKeyPressed(KEY_P)) {
        //     if (System::audio.isMusicPaused()) {
        //         System::audio.pauseAll(false);
        //     } else {
        //         System::audio.pauseAll(true);
        //     }
        // }
        if (IsKeyPressed(KEY_K)) {
            for (auto [entityid, entity] : PlayerSystem::instance()->getEntitiesRef()) {
                entity.kill();
            }
        }
#endif

        // CAMERA
        // -----------------------------------------------------------------------
        // round worldspace coords, keep decimals in screen space
        mWorldSpaceCamera->target.x = static_cast<s32>(mScreenSpaceCamera->target.x);
        mScreenSpaceCamera->target.x -= mWorldSpaceCamera->target.x;
        mScreenSpaceCamera->target.x *= VIRTUAL_SCREEN_RATIO;

        mWorldSpaceCamera->target.y = static_cast<s32>(mScreenSpaceCamera->target.y);
        mScreenSpaceCamera->target.y -= mWorldSpaceCamera->target.y;
        mScreenSpaceCamera->target.y *= VIRTUAL_SCREEN_RATIO;

        // TEXTURE START
        // -----------------------------------------------------------------------
        BeginTextureMode(targetTexture);

        ClearBackground(clearColor);

        TextureManager::instance().drawBackgroundTextures();

        BeginMode2D(*mWorldSpaceCamera);

        spriteSystem->drawEntities();
        drawSystem->drawEntities();

#ifndef NDEBUG
        if (System::input.isDebug()) {
            drawDebugSystem->drawEntities();
            drawColliders();
        }
#endif

        EndMode2D();
        EndTextureMode();
        // -----------------------------------------------------------------------
        // TEXTURE END

        // DRAW START
        // -----------------------------------------------------------------------
        BeginDrawing();

        ClearBackground(clearColor);

        BeginMode2D(*mScreenSpaceCamera);

        Color color = PauseMenu::instance().isPaused() ? Color(25, 50, 75, 255) : WHITE;
        DrawTexturePro(targetTexture.texture, screenSourceRec, screenDestRec, {0.0f, 0.0f}, 0.0f, color);

        EndMode2D();

        // TEXT STUFF
        // DrawTextEx(*mFont, std::format("Camera position: {}", getCameraPosition().toString()).c_str(), Vector2(20, 20), 18, 2, WHITE);
        // int i = 1;
        // for (const auto& lvl : mActiveScene.allLevels) {
        //     auto str = std::format("Level: {}. Origin: {}. Size: {}.", i, (lvl.worldPosOriginTexels * FTEXELS_PER_PIXEL).toString(),
        //                            (lvl.sizeTexels * FTEXELS_PER_PIXEL).toString());
        //     DrawTextEx(*mFont, str.c_str(), Vector2(20, 20 + i * 20), 18, 2, WHITE);
        //     i++;
        // }
        PauseMenu::instance().draw(mFont);

        DrawFPS(10, 10);

        EndDrawing();
        // -----------------------------------------------------------------------
        // DRAW END
    }
    System::schedule.end();
}

void Game::end() {
    System::schedule.await();
    killListeners();

    // raylib stuff:
    TextureManager::instance().unloadAll();
    UnloadFont(*mFont);
    CloseWindow();
}

std::optional<Error> Game::loadScene(const char* filename) {
    if (mIsSceneLoaded) {
        unloadScene();
    }

    auto errOpt = parseWorld(filename, mActiveScene);
    if (errOpt) {
        mIsSceneLoaded = false;
        return errOpt;
    }
    Vector2f cameraPos = toFloatVec(getCameraPosition());
    updateLoadedLevels(cameraPos);
    // TODO spawn player at start pos?

    mIsSceneLoaded = true;
    return std::nullopt;
}

void Game::unloadScene() {
    while (!mActiveScene.loadedLevels.empty()) {
        auto& lvl = mActiveScene.loadedLevels.back();
        unloadLevel(lvl);
        mActiveScene.loadedLevels.pop_back();
    }
    std::set<ecs::Entity> toKill = std::move(mActiveScene.childEntities);
    for (auto entity : toKill) {
        entity.kill();
    }
    mIsSceneLoaded = false;
}

std::optional<Error> Game::reloadScene() {
    auto errOpt = loadScene(mActiveScene.name.c_str());
    if (!errOpt) {
        updateLevelCamera(true);
    }
    return errOpt;
}

Scene& Game::getScene() {
    return mActiveScene;
}

void Game::updateLoadedLevels(Vector2f cameraWorldPosPixels) {
    Vector2f cameraWorldPosTexels = cameraWorldPosPixels * FTEXELS_PER_PIXEL;
    for (auto lvl : mActiveScene.allLevels) {
        Vector2f lvlCenterPos = lvl.worldPosOriginTexels - lvl.sizeTexels * Vector2f(-0.5, 0.5);
        const bool shouldLoad = (lvlCenterPos - cameraWorldPosTexels).len() <= MAX_LOAD_DISTANCE_TEXELS;
        auto it = std::find(mActiveScene.loadedLevels.begin(), mActiveScene.loadedLevels.end(), lvl);
        const bool isLevelLoaded = it != mActiveScene.loadedLevels.end();

        if (shouldLoad && !isLevelLoaded) {
            std::optional<Error> errOpt = loadLevel(lvl);
            if (errOpt) {
                print(*errOpt);
                continue;
            }
        } else if (!shouldLoad && isLevelLoaded) {
            unloadAndRemoveLevel(*it);
        }
    }
}

void Game::updateLevelCamera(bool overrideCache) {
    if (PlayerSystem::instance()->getEntitiesRef().empty() || !mIsSceneLoaded || CameraSystem::instance()->getEntitiesRef().empty()) {
        return;
    }
    static std::string lastLevel = "default";
    std::string curLevel;
    ecs::Entity player = PlayerSystem::instance()->first();
    ecs::Entity camera = CameraSystem::instance()->first();
    Vector2f playerPosTexels = toFloatVec(player.get<Transform2D>().position) * FTEXELS_PER_PIXEL +
                               Vector2f(player.get<ActorCollider>().getCollider().half.x() / 2,
                                        0);  // add halfX so visually the middle of the player has to enter the new level for it to change
    // idk why i have to take half of the half
    auto levelOpt = mActiveScene.getLevelAt(playerPosTexels);

    bool doDefaultCamera = false;
    if (!levelOpt) {
        doDefaultCamera = true;
    } else {
        curLevel = levelOpt.value().filepath;
        if (!overrideCache && curLevel == lastLevel) {
            return;
        }
        lastLevel = curLevel;
        Expected<ActiveLevel*> activeOpt = mActiveScene.getLoadedLevel(levelOpt.value());
        if (!activeOpt.isExpected()) {
            print("Couldn't load level. Got error:", activeOpt.error());
            doDefaultCamera = true;
        } else {
            if (activeOpt.value()->cameraFollow) {
                Follow follow = activeOpt.value()->cameraFollow.value();
                follow.targetEntity = player;
                if (camera.has<Follow>()) {
                    camera.set(follow);
                } else {
                    camera.add(follow);
                }
                return;
            } else {
                Vector2i focalPoint = activeOpt.value()->cameraFocalPoint;
                if (camera.has<Follow>()) {
                    camera.remove<Follow>();
                }
                camera.add(createCameraMoveController(camera.get<Transform2D>().position, focalPoint));
                System::setPaused(true);
                return;
            }
        }
    }
    if (doDefaultCamera) {
        curLevel = "default";
        if (!overrideCache && curLevel == lastLevel) {
            return;
        }
        lastLevel = curLevel;
        Follow follow = Follow(player);
        if (camera.has<Follow>()) {
            camera.set(follow);
        } else {
            camera.add(follow);
        }
    }
}

void Game::loadFont(const char* fontPath, s32 size, s32* codePoints, s32 codePointsCount) {
    *mFont = LoadFontEx(fontPath, size, codePoints, codePointsCount);
}

const Font* Game::getFont() const {
    return mFont;
}

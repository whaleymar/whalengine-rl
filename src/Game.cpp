#include "Game.h"

#include <raylib.h>

#include "ECS/Collision.h"
#include "ECS/Entities/Camera.h"
#include "ECS/Name.h"
#include "ECS/PlayerControl.h"
#include "ECS/RailsControl.h"
#include "ECS/RigidBody.h"
#include "ECS/Systems/Animation.h"
#include "ECS/Systems/CallbackSystem.h"
#include "ECS/Systems/CollisionManager.h"
#include "ECS/Systems/ControllerSystem.h"
#include "ECS/Systems/Gfx.h"
#include "ECS/Systems/Lifetime.h"
#include "ECS/Systems/LightSystem.h"
#include "ECS/Systems/Physics.h"
#include "ECS/Systems/Rails.h"
#include "ECS/Systems/RelationshipManager.h"
#include "ECS/Systems/TagTrackers.h"
#include "ECS/Systems/TriggerSystem.h"
#include "ECS/Tags.h"
#include "ECS/Transform.h"

#include "Events/Events.h"
#include "Events/Listeners.h"

#include "Game/Components/Blaster.h"
#include "Game/DebugScene.h"
#include "Game/Systems/RespawnSystem.h"

#include "Gfx/ShaderManager.h"
#include "Gfx/Texture.h"
#include "Map/Level.h"
#include "Map/Tiled.h"
#include "Settings.h"
#include "Systems/InputHandler.h"
#include "Systems/PauseMenu.h"
#include "Systems/System.h"
#include "Util/Print.h"
#include "Util/Types.h"
#include "Util/Vector.h"

#define NULLOPT Corrade::Containers::NullOpt;

// GAME SETTINGS

constexpr f32 MAX_LOAD_DISTANCE_TEXELS = WINDOW_WIDTH_TEXELS * 3;

// /GAME SETTINGS

using namespace whal;

Game::Game() {
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
    SetExitKey(KEY_NULL);  // Escape quits by default

    // do this before any font/texture stuff or the settings seem to get fucked
    mWorldSpaceCamera->target = Vector2(0.0f, 0.0f);
    mWorldSpaceCamera->offset = Vector2(WINDOW_WIDTH_PIXELS / 2.0f, WINDOW_HEIGHT_PIXELS / 2.0f);  // center camera
    mWorldSpaceCamera->zoom = 1.0f;
    mWorldSpaceCamera->rotation = 0.0f;

    mScreenSpaceCamera->target = Vector2(0.0f, 0.0f);
    mScreenSpaceCamera->zoom = 1.0f;
    mScreenSpaceCamera->rotation = 0.0f;

    loadFont(FONT_PATH, 18, 0, 0);

    Corrade::Containers::Optional<Error> err =
        TextureManager::instance().loadAndRegisterAtlas(SPRITE_TEXTURE_PATH, ATLAS_METADATA_PATH, TEXNAME_SPRITE);
    if (err) {
        print(*err);
        return true;
    }

    err = TextureManager::instance().loadAndRegister(PALETTE_TEXTURE_PATH, TEXNAME_PALETTE);
    if (err) {
        print(*err);
        return true;
    }

    SetTargetFPS(FPS_TARGET);

    if (!System::audio.isValid()) {
        print("Error initializing audio manager");
        return true;
    }
    System::world->setEntityDeathCallback(&emitEntityDeathEvent);
    System::schedule.start();

    // note: nothing is actually running in parallel yet
    System::world->BeginSystemRegistration()
        .parallel<ControllerSystem, FreeControlSystem, JumpSystem>()
        .sequential<PhysicsSystem, RailsSystem, FollowSystem, AttachSystem>()  // all entity movement happens here
        .parallel<TriggerSystem, LifetimeSystem, FadeOutSystem>()
        // .parallel<ProjectileSystem, RocketJumpingSystem>()  // game specific systems
        .parallel<ProjectileSystem>()  // game specific systems
        .parallel<RocketJumpingSystem>(2)
        .parallel<OnFrameEndSystem, AudioListenerSystem, AnimationSystem>();

    System::world->BeginSystemRegistration()
        .registerSystems<DrawSystem, SpriteSystem, DrawTextSystem, DrawDebugSystem, PointLightSystem, BoxLightSystem,
                         RadianceLightSystem>()  // render systems DO have update methods, but are not automated right now bc they're special
        .registerSystems<PlayerSystem, CameraSystem, EntityChildSystem>()
        .registerSystems<QuadTreeSystem>()
        .registerSystems<RespawnListener>();

    // build default components for factory
    err = parseMapProject(TILED_PROJECT_FILE);
    if (err) {
        print("Error parsing ", TILED_PROJECT_FILE, ":", *err);
    }

    return false;
}

void Game::mainloop() {
    // these are used for rendering, which still is run in this function but I might move it eventually
    auto drawSystem = System::world->getSystem<DrawSystem>();
    auto spriteSystem = System::world->getSystem<SpriteSystem>();
    auto textSystem = System::world->getSystem<DrawTextSystem>();
    auto drawDebugSystem = System::world->getSystem<DrawDebugSystem>();
    auto radianceSystem = System::world->getSystem<RadianceLightSystem>();

    // load scene
    auto err = loadTestMap();
    if (err) {
        print("Error loading debug scene: ", *err);
        return;
    }

    System::audio.playMusic("data/audio/music/provingGroundsTheme.mp3");

    // experimenting with adding some extra pixels on border
    RenderTexture2D targetTexture =
        LoadRenderTexture(WINDOW_WIDTH_PIXELS + BLEED_SIZE, WINDOW_HEIGHT_PIXELS + BLEED_SIZE);  // where we'll draw objects to
    RenderTexture2D targetTextureBackground =
        LoadRenderTexture(WINDOW_WIDTH_PIXELS + BLEED_SIZE, WINDOW_HEIGHT_PIXELS + BLEED_SIZE);  // where we'll draw the background to
    RenderTexture2D postProcessTexture = LoadRenderTexture(WINDOW_WIDTH_PIXELS + BLEED_SIZE, WINDOW_HEIGHT_PIXELS + BLEED_SIZE);
    // Color clearColor = {73, 77, 126, 255};
    Color clearColor = {58, 57, 106, 255};
    // Color clearColor = {5, 5, 5, 255};
    Color clearColorTransparent = {0, 0, 0, 0};

    Shader shaderQuantize = ShaderManager::get(Shaders::Quantize);
    auto paletteTexUniform = GetShaderLocation(shaderQuantize, TEXNAME_PALETTE);
    bool isQuantizeOn = false;

    // without the post processing step, would need to flip the y axis here by multiplying by -1
    const Rectangle screenSourceRec = {BLEED_SIZE / 2, BLEED_SIZE / 2, static_cast<f32>(WINDOW_WIDTH_PIXELS),
                                       1 * static_cast<f32>(WINDOW_HEIGHT_PIXELS)};
    const Rectangle screenDestRec = {-VIRTUAL_SCREEN_RATIO, -VIRTUAL_SCREEN_RATIO, WINDOW_WIDTH_ACTUAL + (VIRTUAL_SCREEN_RATIO * 2),
                                     WINDOW_HEIGHT_ACTUAL + (VIRTUAL_SCREEN_RATIO * 2)};

#ifndef NDEBUG
    bool isCreativeMode = false;
#endif

    while (!WindowShouldClose() && !System::isQuit()) {
        System::Update();

        // Update Scene
        checkIfInNewLevel();

        // RENDERING STUFF

#ifndef NDEBUG
        if (IsKeyPressed(KEY_K)) {
            for (auto [entityid, entity] : System::world->getSystem<PlayerSystem>()->getEntitiesRef()) {
                entity.kill();
            }
        }
        if (IsKeyPressed(KEY_R)) {
            reloadScene();
        }
        if (IsKeyPressed(KEY_P)) {
            if (isCreativeMode) {
                isCreativeMode = false;
                for (auto [id, entity] : PlayerSystem::getEntitiesRef()) {
                    entity.remove<FreeControl>();
                    entity.add<RigidBody>();
                    constexpr s32 width = 16;
                    constexpr s32 halfLenX = PIXELS_PER_TEXEL * width / 4;
                    constexpr s32 halfLenY = PIXELS_PER_TEXEL * 6;
                    entity.add(Collider::Actor(entity.get<Transform2D>(), Vector2i(halfLenX, halfLenY)));
                    entity.set(PlayerControl());
                }
            } else {
                isCreativeMode = true;
                for (auto [id, entity] : PlayerSystem::getEntitiesRef()) {
                    entity.add<FreeControl>();
                    entity.remove<RigidBody>();
                    entity.remove<Collider>();
                    entity.set(PlayerControl{250});
                }
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

        // ECS DRAW START
        // -----------------------------------------------------------------------
        drawLights();
        radianceSystem->update();  // this gets drawn to its own texture

        // do backgrounds on their own texture so lighting doesn't affect them
        BeginTextureMode(targetTextureBackground);
        ClearBackground(clearColor);
        TextureManager::instance().drawBackgroundTextures();
        EndTextureMode();

        BeginTextureMode(targetTexture);
        ClearBackground(clearColorTransparent);  // don't overwrite background stuff
        BeginMode2D(*mWorldSpaceCamera);

        spriteSystem->drawEntities();
        drawSystem->drawEntities();
        // textSystem->drawEntities();

        EndMode2D();

        TextureManager::instance().drawLightingTexture();

#ifndef NDEBUG
        BeginMode2D(*mWorldSpaceCamera);
        if (System::input.isOn(InputType::DEBUG)) {
            drawDebugSystem->drawEntities();
            drawColliders();
        }
        EndMode2D();
#endif

        EndTextureMode();
        // -----------------------------------------------------------------------
        // ECS DRAW END
        // Vector2f cameraPosf = getCameraPositionPrecise();
        // auto filename = sprint(cameraPosf, "_.png");
        // Image img = LoadImageFromTexture(targetTexture.texture);
        // ExportImage(img, filename.c_str());

        // POST PROCESSING EFFECTS START
        // -----------------------------------------------------------------------

        BeginTextureMode(postProcessTexture);
        ClearBackground(clearColor);

        if (IsKeyPressed(KEY_Q)) {
            isQuantizeOn = !isQuantizeOn;
        }

        // this unflips the y axis for some reason
        DrawTexture(targetTextureBackground.texture, 0, 0, WHITE);
        DrawTexture(targetTexture.texture, 0, 0, WHITE);

        TextureManager::instance().drawBloomTexture();

        EndTextureMode();

        // -----------------------------------------------------------------------
        // POST PROCESSING EFFECTS END

        // DRAW START
        // -----------------------------------------------------------------------
        BeginDrawing();

        ClearBackground(clearColor);

        BeginMode2D(*mScreenSpaceCamera);

        if (isQuantizeOn) {
            BeginShaderMode(shaderQuantize);
            SetShaderValueTexture(shaderQuantize, paletteTexUniform, TextureManager::instance().getTexture(TEXNAME_PALETTE));
        }

        Color color = PauseMenu::instance().isActive() ? Color(25, 50, 75, 255) : WHITE;
        DrawTexturePro(postProcessTexture.texture, screenSourceRec, screenDestRec, {0.0f, 0.0f}, 0.0f, color);

        if (isQuantizeOn) {
            EndShaderMode();
        }

        textSystem->drawEntities(color);

        EndMode2D();

        // TEXT STUFF
        PauseMenu::instance().draw(mFont);

#ifndef NDEBUG
        if (System::input.isOn(InputType::DEBUG)) {
            DrawFPS(10, 10);
        }
#endif

        EndDrawing();
        // -----------------------------------------------------------------------
        // DRAW END
    }

    UnloadRenderTexture(targetTexture);
    UnloadRenderTexture(targetTextureBackground);
    UnloadRenderTexture(postProcessTexture);
}

void Game::end() {
    System::schedule.end();
    System::schedule.await();

    // raylib stuff:
    TextureManager::instance().unloadAll();
    UnloadFont(*mFont);
    CloseWindow();
}

void Game::onEvent(whal::DeathEvent, ecs::Entity entity) {
    removeEntityFromLevel(entity);
}

void Game::removeEntityFromLevel(ecs::Entity entity) {
    Scene& scene = getScene();
    if (scene.childEntities.erase(entity)) {
        if (entity.has<Name>()) {
            print("erasing entity", entity.get<Name>(), "from child lists");
        }
        return;
    }
    for (auto& lvl : scene.loadedLevels) {
        if (lvl.childEntities.erase(entity)) {
            if (entity.has<Name>()) {
                print("erasing entity", entity.get<Name>(), "from child lists");
            }
            break;
        }
    }
}

Corrade::Containers::Optional<Error> Game::loadScene(const char* filename) {
    if (mIsSceneLoaded) {
        unloadScene();
    }

    auto errOpt = parseWorld(filename, mActiveScene);
    if (errOpt) {
        mIsSceneLoaded = false;
        return errOpt;
    }

    auto eFirstLevel = mActiveScene.loadAndGetFirstLevel();
    if (!eFirstLevel.isExpected()) {
        return eFirstLevel.error();
    }

    Vector2i startPos = eFirstLevel.value()->initialSpawnPoint;

    if (!getCamera()) {
        Vector2i cameraFocus = eFirstLevel.value()->cameraFocalPoint;
        createCamera(Transform2D(cameraFocus));
    } else {
        // by default, camera is at the first level
        Vector2i cameraFocus = eFirstLevel.value()->cameraFocalPoint;

        // but if the player exists, check the level they're in
        if (!PlayerSystem::getEntitiesRef().empty()) {
            auto levelOpt = mActiveScene.getLevelAt(PlayerSystem::first().get<Transform2D>().position);
            if (levelOpt) {
                auto eActiveLvl = mActiveScene.getLoadedLevel(*levelOpt);
                if (eActiveLvl.isExpected()) {
                    cameraFocus = eActiveLvl.value()->cameraFocalPoint;
                }
            }
        }
        setCameraPosition(cameraFocus);
    }
    updateLoadedLevels(toFloatVec(startPos));

    mIsSceneLoaded = true;
    return NULLOPT;
}

void Game::unloadScene() {
    while (!mActiveScene.loadedLevels.empty()) {
        // copy and pop level so the EntityDeathListener doesn't mutate the level we're deleting
        auto lvlCopy = mActiveScene.loadedLevels.back();
        mActiveScene.loadedLevels.pop_back();
        unloadLevel(lvlCopy);
    }
    mActiveScene.startLevelIx = -1;
    clearMapCache();

    std::set<ecs::Entity> toKill = std::move(mActiveScene.childEntities);
    for (auto entity : toKill) {
        entity.kill();
    }
    System::world->killEntities();
    mIsSceneLoaded = false;
}

Corrade::Containers::Optional<Error> Game::reloadScene() {
    auto errOpt = loadScene(mActiveScene.name.c_str());
    if (!errOpt) {
        checkIfInNewLevel(true);
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
        auto it = whal::ecs::whal_find(mActiveScene.loadedLevels.begin(), mActiveScene.loadedLevels.end(), lvl);
        const bool isLevelLoaded = it != mActiveScene.loadedLevels.end();

        if (shouldLoad && !isLevelLoaded) {
            auto errOpt = loadLevel(lvl);
            if (errOpt) {
                print(*errOpt);
                continue;
            }
        } else if (!shouldLoad && isLevelLoaded) {
            unloadAndRemoveLevel(*it);
        }
    }
}

void Game::checkIfInNewLevel(bool overrideCache) {
    if (System::world->getSystem<PlayerSystem>()->getEntitiesRef().empty() || !mIsSceneLoaded ||
        System::world->getSystem<CameraSystem>()->getEntitiesRef().empty()) {
        return;
    }
    static std::string lastLevel = "default";
    std::string curLevel;
    ecs::Entity player = System::world->getSystem<PlayerSystem>()->first();
    // ecs::Entity camera = System::world->getSystem<CameraSystem>()->first();

    Vector2i playerPosition = player.get<Transform2D>().position;
    if (player.has<Collider>()) {
        // add halfX so visually the middle of the player has to enter the new level for it to change
        playerPosition += Vector2i(player.get<Collider>().getShape().getHalf().x(), 0);
    }
    auto levelOpt = mActiveScene.getLevelAt(playerPosition);

    // bool doDefaultCamera = false;
    if (!levelOpt) {
        // doDefaultCamera = true;
    } else {
        curLevel = levelOpt->filepath;
        if (!overrideCache && curLevel == lastLevel) {
            return;
        }
        // IN NEW LEVEL

        Vector2f cameraPos = getCameraPositionPrecise();
        updateLoadedLevels(cameraPos);

        lastLevel = curLevel;
        Expected<ActiveLevel*> activeOpt = mActiveScene.getLoadedLevel(*levelOpt);
        if (!activeOpt.isExpected()) {
            print("Couldn't load level. Got error:", activeOpt.error());
            // doDefaultCamera = true;
        } else {
            System::eventMgr.triggerEvent<EnteredLevelEvent>(player, *activeOpt.value());
        }
    }
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

void Game::loadFont(const char* fontPath, s32 size, s32* codePoints, s32 codePointsCount) {
    *mFont = LoadFontEx(fontPath, size, codePoints, codePointsCount);
}

const Font* Game::getFont() const {
    return mFont;
}

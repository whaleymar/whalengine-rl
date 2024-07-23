#include "Game.h"

#include <raylib.h>

#include "Components/Collision.h"
#include "Components/Name.h"
#include "Components/PlayerControl.h"
#include "Components/RailsControl.h"
#include "Components/RigidBody.h"
#include "Components/Tags.h"
#include "Components/Transform.h"
#include "Entities/Camera.h"
#include "Systems/Animation.h"
#include "Systems/CallbackSystem.h"
#include "Systems/CollisionManager.h"
#include "Systems/ControllerSystem.h"
#include "Systems/Gfx.h"
#include "Systems/Lifetime.h"
#include "Systems/LightSystem.h"
#include "Systems/ParticleEmitterSystem.h"
#include "Systems/Physics.h"
#include "Systems/Rails.h"
#include "Systems/RelationshipManager.h"
#include "Systems/TagTrackers.h"
#include "Systems/TriggerSystem.h"

#include "Events/Events.h"
#include "Events/Listeners.h"

#include "Game/Components/Blaster.h"
#include "Game/Components/Respawn.h"
#include "Game/Entities/Player.h"
#include "Game/Save/EventFlags.h"
#include "Game/Systems/RespawnSystem.h"
#include "Settings.h"

#include "Gfx/ShaderManager.h"
#include "Gfx/Texture.h"

#include "Map/Level.h"
#include "Map/Tiled.h"

#include "Sys/InputHandler.h"
#include "Sys/PauseMenu.h"
#include "Sys/System.h"

#include "Util/Print.h"
#include "Util/Types.h"
#include "Util/Vector.h"

#define NULLOPT Corrade::Containers::NullOpt;

// GAME SETTINGS

constexpr f32 MAX_LOAD_DISTANCE_TEXELS = WINDOW_WIDTH_TEXELS * 3;
const char* SCENE_FILE = "world1.world";

// /GAME SETTINGS

// STATIC VARS

static Image S_ICON_IMAGE;

// /STATIC VARS

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

    S_ICON_IMAGE = LoadImage("data/icon-hat.png");
    SetWindowIcon(S_ICON_IMAGE);

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
        .sequential<RotationPhysicsSystem, PhysicsSystem, RailsSystem, FollowSystem, AttachSystem, OrbitSystem>()  // all entity movement happens here
        // .sequential<RotationPhysicsSystem, PhysicsSystem, RailsSystem, FollowSystem, AttachSystem>()  // all entity movement happens here
        .parallel<TriggerSystem, LifetimeSystem, FadeOutSystem, ColorLerpSystem, ScaleLerpSystem>()
        .parallel<ParticleEmitterSystem, ProjectileSystem, CustomUpdateSystem>()
        .parallel<RocketJumpingSystem, SlowEntityKillerSystem>(2)
        .parallel<OnFrameEndSystem, AudioListenerSystem, AnimationSystem>();

    System::world->BeginSystemRegistration()
        .registerSystems<GfxSystem, DrawTextSystem, DrawDebugSystem, PointLightSystem, BoxLightSystem,
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
    auto gfxSystem = System::world->getSystem<GfxSystem>();
    auto textSystem = System::world->getSystem<DrawTextSystem>();
    auto radianceSystem = System::world->getSystem<RadianceLightSystem>();

    // load scene
    // auto err = loadTestMap();
    auto err = loadScene(SCENE_FILE, true);
    if (err) {
        print("Error loading debug scene: ", *err);
        return;
    }

    // Color clearColor = {73, 77, 126, 255};
    Color clearColor = {58, 57, 106, 255};
    // Color clearColor = {5, 5, 5, 255};

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
            for (auto [entityid, entity] : System::world->getSystem<PlayerSystem>()->getEntitiesMutable()) {
                entity.kill();
            }
        }
        if (IsKeyPressed(KEY_P)) {
            if (isCreativeMode) {
                isCreativeMode = false;
                for (auto [id, entity] : PlayerSystem::getEntitiesMutable()) {
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
                for (auto [id, entity] : PlayerSystem::getEntitiesMutable()) {
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

        // todo move begin/endTextureMode functions and background functions that only call one function to the called function

        // do backgrounds on their own texture so lighting doesn't affect them
        BeginTextureMode(TextureManager::getRenderTexture(TextureID::Background));
        ClearBackground(clearColor);
        TextureManager::instance().drawBackgroundTextures();
        EndTextureMode();

        gfxSystem->drawEntities();

        // -----------------------------------------------------------------------
        // ECS DRAW END
        // Vector2f cameraPosf = getCameraPositionPrecise();
        // auto filename = sprint(cameraPosf, "_.png");
        // Image img = LoadImageFromTexture(targetTexture.texture);
        // ExportImage(img, filename.c_str());

        // POST PROCESSING EFFECTS START
        // -----------------------------------------------------------------------

        BeginTextureMode(TextureManager::getRenderTexture(TextureID::PostProcess));
        ClearBackground(clearColor);

        if (IsKeyPressed(KEY_Q)) {
            isQuantizeOn = !isQuantizeOn;
        }

        // this unflips the y axis for some reason
        DrawTexture(TextureManager::getRenderTexture(TextureID::Background).texture, 0, 0, WHITE);
        DrawTexture(TextureManager::getRenderTexture(TextureID::Main).texture, 0, 0, WHITE);

        TextureManager::instance().drawRadianceTexture();

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

        // looks cool, but is too much to apply it to the whole scene
        // BeginShaderMode(ShaderManager::get(Shaders::Bloom));
        DrawTexturePro(TextureManager::getRenderTexture(TextureID::PostProcess).texture, screenSourceRec, screenDestRec, {0.0f, 0.0f}, 0.0f, color);
        // EndShaderMode();

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
}

void Game::end() {
    System::schedule.end();
    System::schedule.await();

    // raylib stuff:
    TextureManager::instance().unloadAll();
    UnloadFont(*mFont);
    UnloadImage(S_ICON_IMAGE);
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

Corrade::Containers::Optional<Error> Game::loadScene(const char* filename, bool resetPlayers) {
    if (mIsSceneLoaded) {
        unloadScene(resetPlayers);
    }

    System::audio.playMusic("data/audio/music/provingGroundsTheme.mp3");

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

    if (resetPlayers || PlayerSystem::getEntitiesMutable().empty()) {
        createPlayer();
    }

    if (!getCamera()) {
        Vector2i cameraFocus = eFirstLevel.value()->cameraFocalPoint;
        createCamera(Transform2D(cameraFocus));
    } else {
        // by default, camera is at the first level
        Vector2i cameraFocus = eFirstLevel.value()->cameraFocalPoint;

        // but if the player exists, check the level they're in
        if (!PlayerSystem::getEntitiesMutable().empty()) {
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
    updateLoadedLevels(startPos.as<f32>());

    mIsSceneLoaded = true;
    return NULLOPT;
}

void Game::unloadScene(bool resetPlayers) {
    System::audio.stopAll();
    EventFlags::resetAll();  // TODO should not live here

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
        if (entity.has<Respawn>()) {
            entity.remove<Respawn>();
        }
        entity.kill();
    }

    if (resetPlayers) {
        for (auto [entityid, entity] : PlayerSystem::getEntitiesCopy()) {
            if (entity.has<Respawn>()) {
                entity.remove<Respawn>();
            }
            entity.kill();
        }
    }

    System::world->killEntities();
    mIsSceneLoaded = false;
}

Corrade::Containers::Optional<Error> Game::reloadScene(bool resetPlayers) {
    auto errOpt = loadScene(mActiveScene.name.c_str(), resetPlayers);
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
    if (System::world->getSystem<PlayerSystem>()->getEntitiesMutable().empty() || !mIsSceneLoaded ||
        System::world->getSystem<CameraSystem>()->getEntitiesMutable().empty()) {
        return;
    }
    static std::string lastLevel = "default";
    std::string curLevel;
    ecs::Entity player = System::world->getSystem<PlayerSystem>()->first();
    // ecs::Entity camera = System::world->getSystem<CameraSystem>()->first();

    Vector2i playerPosition = player.get<Transform2D>().position;
    if (player.has<Collider>()) {
        // add halfX so visually the middle of the player has to enter the new level for it to change
        playerPosition += Vector2i(player.get<Collider>().getShape().getHalf().x, 0);
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
            (*activeOpt)->activateObjects();
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

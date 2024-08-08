#include "Game.h"

#include <raylib.h>

#include "Components/Collision.h"
#include "Components/Name.h"
#include "Components/PlayerControl.h"
#include "Components/RailsControl.h"
#include "Components/RigidBody.h"
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

#include "Gfx/Pipeline.h"
#include "Gfx/ShaderManager.h"
#include "Gfx/Texture.h"

#include "Map/Level.h"
#include "Map/Tiled.h"

#include "Sys/InputHandler.h"
#include "Sys/PauseMenu.h"
#include "Sys/System.h"

#include "Systems/TweenSystem.h"
#include "Util/Print.h"
#include "Util/Types.h"
#include "Util/Vector.h"

#define NULLOPT Corrade::Containers::NullOpt;

// WEB BUILD STUFF

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

// /WEB

// GAME SETTINGS

constexpr f32 MAX_LOAD_DISTANCE_TEXELS = WINDOW_WIDTH_TEXELS * 3;
const char* SCENE_FILE = "world1.world";

static Camera2D S_CAMERA_WORLDSPACE;
static Camera2D S_CAMERA_SCREENSPACE;
static Font S_FONT_PAUSEMENU;

// /GAME SETTINGS

using namespace whal;

// MAINLOOP VARIABLES

// gfx stuff
static GfxSystem* gfxSystem = nullptr;
static DrawTextSystem* textSystem = nullptr;
static RadianceLightSystem* radianceSystem = nullptr;
static Pipeline* postProcessPipeline = nullptr;

// hopefully can remove eventually:
Shader shaderQuantize;
s32 paletteTexUniform;
static bool isQuantizeOn = false;

#ifndef NDEBUG
static bool isCreativeMode = false;  // known issue: killing player when in creative mode will cause crash on next creative mode activation
#endif

// /MAINLOOP VARIABLES

bool Game::start() {
    // do this before any font/texture stuff or the settings seem to get fucked
    S_CAMERA_WORLDSPACE.target = Vector2(0.0f, 0.0f);
    S_CAMERA_WORLDSPACE.offset = Vector2(WINDOW_WIDTH_PIXELS / 2.0f, WINDOW_HEIGHT_PIXELS / 2.0f);  // center camera
    S_CAMERA_WORLDSPACE.zoom = 1.0f;
    S_CAMERA_WORLDSPACE.rotation = 0.0f;

    S_CAMERA_SCREENSPACE.target = Vector2(0.0f, 0.0f);
    S_CAMERA_SCREENSPACE.zoom = 1.0f;
    S_CAMERA_SCREENSPACE.rotation = 0.0f;

    S_FONT_PAUSEMENU = LoadFontEx(FONT_PATH, 18, 0, 0);

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

    // RESEARCH considering moving registration for engine-specific systems into Engine.start, but that would make it harder to edit them (would
    // require recompiling), so maybe not?
    // note: nothing is actually running in parallel yet
    System::world.BeginSystemRegistration()
        .parallel<ControllerSystem, FreeControlSystem, JumpSystem>()
        .sequential<RotationPhysicsSystem, PhysicsSystem, RailsSystem, FollowSystem, AttachSystem, OrbitSystem,
                    TweenPositionSystem>()  // all entity movement happens here
        .parallel<TriggerSystem, LifetimeSystem, FadeOutSystem, ColorLerpSystem, ScaleLerpSystem>()
        .parallel<ParticleEmitterSystem, ProjectileSystem, CustomUpdateSystem>()
        .parallel<RocketJumpingSystem, SlowEntityKillerSystem>(2)
        .parallel<OnFrameEndSystem, AudioListenerSystem, AnimationSystem>();

    System::world.BeginSystemRegistration()
        .registerSystems<GfxSystem, DrawTextSystem, DrawDebugSystem, PointLightSystem, BoxLightSystem, RadianceLightSystem,
                         ShadowLightSystem>()  // render systems DO have update methods, but are not automated right now bc they're special
        .registerSystems<PlayerSystem, CameraSystem, EntityChildSystem>()
        .registerSystems<QuadTreeSystem>()
        .registerSystems<RespawnListener>();

    gfxSystem = System::world.getSystem<GfxSystem>();
    textSystem = System::world.getSystem<DrawTextSystem>();
    radianceSystem = System::world.getSystem<RadianceLightSystem>();

    // graphics stuff:
    shaderQuantize = ShaderManager::get(Shaders::Quantize);
    paletteTexUniform = GetShaderLocation(shaderQuantize, "iPalette");
    postProcessPipeline = new Pipeline({WINDOW_WIDTH_PIXELS, WINDOW_HEIGHT_PIXELS}, {
                                                                                        // Shaders::Bloom,
                                                                                        // Shaders::Glitch,
                                                                                        // Shaders::Quantize,
                                                                                    });

    // build default components for factory
    parseMapProject(TILED_PROJECT_FILE);

    // load first scene:
    err = loadScene(SCENE_FILE, true);
    if (err) {
        print("Error loading debug scene: ", *err);
        return true;
    }

    return false;
}

// without the post processing step, would need to flip the y axis here by multiplying by -1
static const Rectangle SCREEN_SOURCE_RECT = {BLEED_SIZE / 2, BLEED_SIZE / 2, static_cast<f32>(WINDOW_WIDTH_PIXELS),
                                             1 * static_cast<f32>(WINDOW_HEIGHT_PIXELS)};
static const Rectangle SCREEN_DEST_RECT = {-VIRTUAL_SCREEN_RATIO, -VIRTUAL_SCREEN_RATIO, WINDOW_WIDTH_ACTUAL + (VIRTUAL_SCREEN_RATIO * 2),
                                           WINDOW_HEIGHT_ACTUAL + (VIRTUAL_SCREEN_RATIO * 2)};

// the update loop is in its own function bc emscripten
void Update() {
    System::Update();

    // Update Scene
    Game::instance().checkIfInNewLevel();

    // RENDERING STUFF

#ifndef NDEBUG
    if (IsKeyPressed(KEY_K)) {
        for (auto [entityid, entity] : System::world.getSystem<PlayerSystem>()->getEntitiesMutable()) {
            entity.kill();
        }
    }
    if (IsKeyPressed(KEY_P)) {
        if (isCreativeMode) {
            isCreativeMode = false;
            for (auto [id, entity] : PlayerSystem::getEntitiesMutable()) {
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
    // TODO do these need to be updated every frame? or can they be const?
    // TODO should also look into moving camera instead of always offsetting objects. Wonder if that would help with graphical artifacts?
    S_CAMERA_WORLDSPACE.target.x = static_cast<s32>(S_CAMERA_SCREENSPACE.target.x);
    S_CAMERA_SCREENSPACE.target.x -= S_CAMERA_WORLDSPACE.target.x;
    S_CAMERA_SCREENSPACE.target.x *= VIRTUAL_SCREEN_RATIO;

    S_CAMERA_WORLDSPACE.target.y = static_cast<s32>(S_CAMERA_SCREENSPACE.target.y);
    S_CAMERA_SCREENSPACE.target.y -= S_CAMERA_WORLDSPACE.target.y;
    S_CAMERA_SCREENSPACE.target.y *= VIRTUAL_SCREEN_RATIO;

    // ECS DRAW START
    // -----------------------------------------------------------------------
    radianceSystem->update(S_CAMERA_WORLDSPACE);            // drawn to TextureID::Radiance
    TextureManager::instance().renderBackgroundTextures();  // drawn to TextureID::Background
    gfxSystem->drawEntities(S_CAMERA_WORLDSPACE);           // draws entities AND backgrounds
    drawLights(S_CAMERA_WORLDSPACE);                        // drawn to TextureID::Lighting

    // -----------------------------------------------------------------------
    // ECS DRAW END

    // POST PROCESSING EFFECTS START
    // -----------------------------------------------------------------------

    BeginTextureMode(TextureManager::getRenderTexture(TextureID::PostProcess));
    ClearBackground(BLACK);

    if (IsKeyPressed(KEY_Q)) {
        isQuantizeOn = !isQuantizeOn;
    }

    // this unflips the y axis (RenderTextures are drawn upside down by default because raylib is stupid)
    DrawTexture(TextureManager::getRenderTexture(TextureID::Main).texture, 0, 0, WHITE);
    // DrawTexture(TextureManager::getRenderTexture(TextureID::Occlusion).texture, 0, 0, WHITE); // testing
    TextureManager::instance().drawLightingTexture();
    TextureManager::instance().drawRadianceTexture();
    EndTextureMode();

    postProcessPipeline->process(TextureID::PostProcess);

    // -----------------------------------------------------------------------
    // POST PROCESSING EFFECTS END

    // DRAW START
    // -----------------------------------------------------------------------
    BeginDrawing();

    ClearBackground(Colors::Clear);

    BeginMode2D(S_CAMERA_SCREENSPACE);

    // TODO i want this to be in the gfx pipeline, but having trouble setting the palette texture uniform -- works after the *first* time i press
    // Q, but is completely black before that
    if (isQuantizeOn) {
        BeginShaderMode(shaderQuantize);
        SetShaderValueTexture(shaderQuantize, paletteTexUniform, TextureManager::instance().getTexture(TEXNAME_PALETTE));
    }

    Color color = PauseMenu::instance().isActive() ? Color(25, 50, 75, 255) : WHITE;

    DrawTexturePro(TextureManager::getRenderTexture(TextureID::PostProcess).texture, SCREEN_SOURCE_RECT, SCREEN_DEST_RECT, {0.0f, 0.0f}, 0.0f, color);

    if (isQuantizeOn) {
        EndShaderMode();
    }

    textSystem->drawEntities(color);

    EndMode2D();

    // TEXT STUFF
    PauseMenu::instance().draw(S_FONT_PAUSEMENU);

#ifndef NDEBUG
    if (System::input.isOn(InputType::DEBUG)) {
        DrawFPS(10, 10);
    }
#endif
    // DrawFPS(10, 10); // for testing in release build

    EndDrawing();
    // -----------------------------------------------------------------------
    // DRAW END
}

void Game::mainloop() {
#ifdef __EMSCRIPTEN__
    EM_ASM(FS.mkdir('/work'); FS.mount(IDBFS, {}, '/work'); FS.syncfs(true, function(err) { assert(!err); }););
    System::time.sleep(1);
    emscripten_set_main_loop(Update, 0, 1);  // arg1: tells browser to control FPS. arg2: tells browser to simulate infinite loop for us
#else
    while (!WindowShouldClose() && !System::isQuit()) {
        Update();
    }
#endif
}

void Game::end() {
    delete postProcessPipeline;
    TextureManager::instance().unloadAll();
    UnloadFont(S_FONT_PAUSEMENU);
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

    System::world.killEntities();
    mIsSceneLoaded = false;
}

Corrade::Containers::Optional<Error> Game::reloadScene(bool resetPlayers) {
    auto errOpt = loadScene(mActiveScene.name.c_str(), resetPlayers);
    if (!errOpt) {
        checkIfInNewLevel(true);
    }

    // #ifndef NDEBUG
    // TODO for this to work i think i need to reload all uniforms, which sucks bc they're all decentralized
    // ShaderManager::instance().unloadAll();
    // ShaderManager::instance().loadShaders();
    // #endif

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
    if (System::world.getSystem<PlayerSystem>()->getEntitiesMutable().empty() || !mIsSceneLoaded ||
        System::world.getSystem<CameraSystem>()->getEntitiesMutable().empty()) {
        return;
    }
    static std::string lastLevel = "default";
    std::string curLevel;
    ecs::Entity player = System::world.getSystem<PlayerSystem>()->first();
    // ecs::Entity camera = System::world.getSystem<CameraSystem>()->first();

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

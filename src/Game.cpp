#include "Game.h"

#include <raylib.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "ECS/Collision.h"
#include "ECS/Entities/Camera.h"
#include "ECS/Name.h"
#include "ECS/RailsControl.h"
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
#include "ECS/Transform.h"

#include "Events/Listeners.h"

#include "Game/Components/Blaster.h"
#include "Game/DebugScene.h"
#include "Game/Systems/RespawnSystem.h"

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

#ifdef __EMSCRIPTEN__
EM_JS(void, idbfs_put, (const char* filename, const char* str), {
    FS.writeFile(UTF8ToString(filename), UTF8ToString(str));
    FS.syncfs(
        false, function(err) { assert(!err); });
});
EM_JS(char*, idbfs_get, (const char* filename), {
    var arr = FS.readFile(UTF8ToString(filename));
    var jsString = new TextDecoder().decode(arr);
    var lengthBytes = lengthBytesUTF8(jsString) + 1;
    // console.log(jsString);
    var stringOnWasmHeap = _malloc(lengthBytes);
    stringToUTF8(jsString, stringOnWasmHeap, lengthBytes);
    return stringOnWasmHeap;
});
#endif

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

// these are used for rendering, which still is run manually in the main loop but I might move it eventually
static DrawSystem* drawSystem;
static SpriteSystem* spriteSystem;
static DrawDebugSystem* drawDebugSystem;
static PointLightSystem* lightSystem;
static RadianceLightSystem* radianceSystem;

static CollisionManager* collisionMgr;
// should be its own phase

// experimenting with adding some extra pixels on border (for putting camera in screen space)
static RenderTexture2D targetTexture;
static RenderTexture2D targetTextureBackground;
static RenderTexture2D targetTextureRadiance;
static RenderTexture2D postProcessTexture;

static const Color clearColor = {5, 5, 5, 255};
static const Color clearColorTransparent = {0, 0, 0, 0};

static Shader shaderPointLight;
static int lightPosUniform;
static Shader shaderRadiance;
static int radiancePosUniform;

static Shader shaderQuantize;
static int paletteTexUniform;
static bool isQuantizeOn = false;

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

    // if (!System::audio.isValid()) {
    //     print("Error initializing audio manager");
    //     return true;
    // }
    System::world->setEntityDeathCallback(&emitEntityDeathEvent);
    System::schedule.start();

    // note: nothing is actually running in parallel yet
    System::world->BeginSystemRegistration()
        .parallel<ControllerSystem, FreeControlSystem, JumpSystem>()
        .sequential<PhysicsSystem, RailsSystem, FollowSystem, AttachSystem>()  // all entity movement happens here
        .parallel<TriggerSystem, LifetimeSystem>()
        .sequential<ProjectileSystem>()  // game specific systems
        .parallel<OnFrameEndSystem, AudioListenerSystem, AnimationSystem>();

    System::world->BeginSystemRegistration()
        .registerSystems<DrawSystem, SpriteSystem, DrawDebugSystem, PointLightSystem,
                         RadianceLightSystem>()  // render systems DO have update methods, but are not automated right now bc they're special
        .registerSystems<PlayerSystem, CameraSystem, EntityChildSystem, MovableColliders>()
        .registerSystems<RocketJumpingSystem, RespawnListener>();

    // these are used for rendering, which still is run manually in the main loop but I might move it eventually
    drawSystem = System::world->getSystem<DrawSystem>();
    spriteSystem = System::world->getSystem<SpriteSystem>();
    drawDebugSystem = System::world->getSystem<DrawDebugSystem>();
    lightSystem = System::world->getSystem<PointLightSystem>();
    radianceSystem = System::world->getSystem<RadianceLightSystem>();

    collisionMgr = CollisionManager::instance();  // not registering this with the others because i want it to update during rendering,
    // which should be its own phase

    // experimenting with adding some extra pixels on border (for putting camera in screen space)
    targetTexture = LoadRenderTexture(WINDOW_WIDTH_PIXELS + BLEED_SIZE, WINDOW_HEIGHT_PIXELS + BLEED_SIZE);  // where we'll draw objects to
    targetTextureBackground =
        LoadRenderTexture(WINDOW_WIDTH_PIXELS + BLEED_SIZE, WINDOW_HEIGHT_PIXELS + BLEED_SIZE);  // where we'll draw the background to
    targetTextureRadiance =
        LoadRenderTexture(WINDOW_WIDTH_PIXELS + BLEED_SIZE, WINDOW_HEIGHT_PIXELS + BLEED_SIZE);  // where we'll draw the background to
    postProcessTexture = LoadRenderTexture(WINDOW_WIDTH_PIXELS + BLEED_SIZE, WINDOW_HEIGHT_PIXELS + BLEED_SIZE);
    // static Color clearColor = {51, 76, 76, 255};

#ifdef __EMSCRIPTEN__
    shaderPointLight = LoadShader(0, "src/Shader/pointlight-web.glsl");
    shaderRadiance = LoadShader(0, "src/Shader/radiancelight-web.glsl");
    shaderQuantize = LoadShader(0, "src/Shader/quantize-web.glsl");
#else
    shaderPointLight = LoadShader(0, "src/Shader/pointlight.glsl");
    shaderRadiance = LoadShader(0, "src/Shader/radiancelight.glsl");
    shaderQuantize = LoadShader(0, "src/Shader/quantize.fs");
#endif

    lightPosUniform = GetShaderLocation(shaderPointLight, "position");
    radiancePosUniform = GetShaderLocation(shaderRadiance, "position");
    paletteTexUniform = GetShaderLocation(shaderQuantize, TEXNAME_PALETTE);

    return false;
}

static void _mainloop();

// without the post processing step, would need to flip the y axis here by multiplying by -1
static const Rectangle screenSourceRec = {BLEED_SIZE / 2, BLEED_SIZE / 2, static_cast<f32>(WINDOW_WIDTH_PIXELS),
                                          1 * static_cast<f32>(WINDOW_HEIGHT_PIXELS)};
static const Rectangle screenDestRec = {-VIRTUAL_SCREEN_RATIO, -VIRTUAL_SCREEN_RATIO, WINDOW_WIDTH_ACTUAL + (VIRTUAL_SCREEN_RATIO * 2),
                                        WINDOW_HEIGHT_ACTUAL + (VIRTUAL_SCREEN_RATIO * 2)};

void Game::mainloop() {
    // load scene
    auto err = loadTestMap();
    if (err) {
        print("Error loading debug scene: ", *err);
        return;
    }

    // System::audio.playMusic("data/audio/music/provingGroundsTheme.mp3");

    collisionMgr->update();

    lightSystem->setShader(&shaderPointLight);
    lightSystem->setPositionUniform(lightPosUniform);

    radianceSystem->setShader(&shaderRadiance);
    radianceSystem->setPositionUniform(radiancePosUniform);

#ifdef __EMSCRIPTEN__

    EM_ASM(FS.mkdir('/work'); FS.mount(IDBFS, {}, '/work'); FS.syncfs(
        true, function(err) { assert(!err); }););
    System::dt.sleep(1);
    idbfs_put("file.txt", "Some dynamic file contents...\n");
    EM_ASM({ Module.wasmTable = wasmTable; });
    emscripten_set_main_loop(_mainloop, 0, 0);

#else
    while (!WindowShouldClose() && !System::isQuit()) {
        _mainloop();
    }
#endif
}

// required for web builds
static void _mainloop() {
    System::input.update();
    if (System::frame.getFrame() == 0) {
        Vector2f cameraPos = getCameraPositionPrecise();
        Game::instance().updateLoadedLevels(cameraPos);
    }

    System::dt.update();
    System::schedule.tick(System::dt());
    System::frame.update();
    // System::audio.update();

    System::world->update();

    // Update Scene
    Game::instance().checkIfInNewLevel();

    // Only rendering remains, so we can do "end of frame" stuff now
    System::world->killEntities();
    collisionMgr->update();  // this can definitely be done in parallel while rendering

#ifndef NDEBUG
    if (IsKeyPressed(KEY_K)) {
        for (auto [entityid, entity] : System::world->getSystem<PlayerSystem>()->getEntitiesRef()) {
            entity.kill();
        }
    }
#endif

    // CAMERA
    // -----------------------------------------------------------------------
    // round worldspace coords, keep decimals in screen space
    auto pWorldSpaceCamera = Game::instance().getWorldCamera();
    auto pScreenSpaceCamera = Game::instance().getScreenCamera();

    pWorldSpaceCamera->target.x = static_cast<s32>(pScreenSpaceCamera->target.x);
    pScreenSpaceCamera->target.x -= pWorldSpaceCamera->target.x;
    pScreenSpaceCamera->target.x *= VIRTUAL_SCREEN_RATIO;

    pWorldSpaceCamera->target.y = static_cast<s32>(pScreenSpaceCamera->target.y);
    pScreenSpaceCamera->target.y -= pWorldSpaceCamera->target.y;
    pScreenSpaceCamera->target.y *= VIRTUAL_SCREEN_RATIO;

    // ECS DRAW START
    // -----------------------------------------------------------------------
    lightSystem->update();  // this gets drawn to its own texture
    BeginTextureMode(targetTextureRadiance);
    ClearBackground(clearColorTransparent);  // don't overwrite background stuff
    radianceSystem->update();                // draws to current texture
    EndTextureMode();

    // do backgrounds on their own texture so lighting doesn't affect them
    BeginTextureMode(targetTextureBackground);
    ClearBackground(clearColor);
    TextureManager::instance().drawBackgroundTextures();
    EndTextureMode();

    BeginTextureMode(targetTexture);
    ClearBackground(clearColorTransparent);  // don't overwrite background stuff
    BeginMode2D(*pWorldSpaceCamera);

    spriteSystem->drawEntities();
    drawSystem->drawEntities();

    EndMode2D();

    BeginBlendMode(BLEND_MULTIPLIED);
    TextureManager::instance().drawLightingTexture();
    EndBlendMode();

#ifndef NDEBUG
    BeginMode2D(*pWorldSpaceCamera);
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
    if (isQuantizeOn) {
        BeginShaderMode(shaderQuantize);

        SetShaderValueTexture(shaderQuantize, paletteTexUniform, TextureManager::instance().getTexture(TEXNAME_PALETTE));
    }

    // this unflips the y axis for some reason
    DrawTexture(targetTextureBackground.texture, 0, 0, WHITE);
    DrawTexture(targetTexture.texture, 0, 0, WHITE);

    BeginBlendMode(BLEND_ADDITIVE);
    DrawTexture(targetTextureRadiance.texture, 0, 0, WHITE);
    EndBlendMode();

    if (isQuantizeOn)
        EndShaderMode();
    EndTextureMode();

    // -----------------------------------------------------------------------
    // POST PROCESSING EFFECTS END

    // DRAW START
    // -----------------------------------------------------------------------
    BeginDrawing();

    ClearBackground(clearColor);

    BeginMode2D(*pScreenSpaceCamera);

    Color color = PauseMenu::instance().isActive() ? Color(25, 50, 75, 255) : WHITE;
    DrawTexturePro(postProcessTexture.texture, screenSourceRec, screenDestRec, {0.0f, 0.0f}, 0.0f, color);

    EndMode2D();

    // TEXT STUFF
    PauseMenu::instance().draw(Game::instance().getFont());

#ifndef NDEBUG
    if (System::input.isOn(InputType::DEBUG)) {
        DrawFPS(10, 10);
    }
#endif

    EndDrawing();
    // -----------------------------------------------------------------------
    // DRAW END
}

void Game::end() {
    UnloadRenderTexture(targetTexture);
    UnloadRenderTexture(targetTextureBackground);
    UnloadRenderTexture(targetTextureRadiance);
    UnloadRenderTexture(postProcessTexture);

    System::schedule.end();
    System::schedule.await();

    // raylib stuff:
    TextureManager::instance().unloadAll();
    UnloadFont(*mFont);
    CloseWindow();
}

void Game::onEvent(ecs::Entity entity) {
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

    Vector2i startPos = mActiveScene.loadSceneAndGetStartPosition();
    setCameraPosition(startPos);
    updateLoadedLevels(toFloatVec(startPos));

    mIsSceneLoaded = true;
    return NULLOPT;
}

void Game::unloadScene() {
    while (!mActiveScene.loadedLevels.empty()) {
        auto& lvl = mActiveScene.loadedLevels.back();
        unloadLevel(lvl);
        mActiveScene.loadedLevels.pop_back();
    }
    mActiveScene.startLevelIx = -1;

    std::set<ecs::Entity> toKill = std::move(mActiveScene.childEntities);
    for (auto entity : toKill) {
        entity.kill();
    }
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
    ecs::Entity camera = System::world->getSystem<CameraSystem>()->first();

    // add halfX so visually the middle of the player has to enter the new level for it to change
    Vector2i playerPosition = player.get<Transform2D>().position + Vector2i(player.get<Collider>().getShape().getHalf().x(), 0);
    auto levelOpt = mActiveScene.getLevelAt(playerPosition);

    bool doDefaultCamera = false;
    if (!levelOpt) {
        doDefaultCamera = true;
    } else {
        curLevel = levelOpt->filepath;
        if (!overrideCache && curLevel == lastLevel) {
            return;
        }
        // IN NEW LEVEL
        lastLevel = curLevel;
        Expected<ActiveLevel*> activeOpt = mActiveScene.getLoadedLevel(*levelOpt);
        if (!activeOpt.isExpected()) {
            print("Couldn't load level. Got error:", activeOpt.error());
            doDefaultCamera = true;
        } else {
            System::eventMgr.triggerEvent<EnteredLevelEvent>(player, *activeOpt.value());
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

Font* Game::getFont() const {
    return mFont;
}

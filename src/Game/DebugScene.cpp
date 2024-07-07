#include "Game/DebugScene.h"

#include <raylib.h>

#include "Settings.h"
#include "Util/Print.h"
#include "whalECS/src/ECS.h"

#include "Game.h"
#include "Gfx/Texture.h"
#include "Systems/System.h"

#include "ECS/Callback.h"
#include "ECS/Collision.h"
#include "ECS/Draw.h"
#include "ECS/Entities/Block.h"
#include "ECS/Entities/Camera.h"
#include "ECS/Entities/Player.h"
#include "ECS/Name.h"
#include "ECS/RailsControl.h"
#include "ECS/RigidBody.h"
#include "ECS/Transform.h"
#include "ECS/TriggerZone.h"
#include "ECS/Velocity.h"

void createTestPlatform();
void createTestTrigger();
void createTestSemiSolid();
void createDepthTest();
void createTestMouseTracker();
void createPaletteTest();

using namespace whal;

Corrade::Containers::Optional<Error> loadMap() {
    // const char* scenefile = "testworld.world";
    const char* scenefile = "world1.world";
    return Game::instance().loadScene(scenefile);
}

Corrade::Containers::Optional<Error> loadTestMap() {
    // createTestPlatform();
    auto err = loadMap();
    // createTestPlatform();
    // createTestMouseTracker();
    // createTestTrigger();
    // createTestSemiSolid();
    // createDepthTest();
    // createPaletteTest();

    auto ePlayer = createPlayer();
    if (!ePlayer.isExpected()) {
        return ePlayer.error();
    }

    return err;
}

Corrade::Containers::Optional<Error> loadDebugScene() {
    // auto player = createPlayer().value();
    // player.remove<PlayerControlRB>();

    auto player = createPlayer();

    // auto playerCopyExpected = createPlayer();
    // // player.value().kill();
    // if (playerCopyExpected.isExpected()) {
    //     auto playerCopy = playerCopyExpected.value();
    //     playerCopy.set(Transform::tiles(20, 10));
    //     playerCopy.get<Sprite>().setColor(Color::EMERALD);
    // } else {
    //     return playerCopyExpected.error();
    // }

    for (s32 i = 0; i < 50; i++) {
        createBlock(Transform2D::tiles(i, 1));
    }

    // auto tmp = createBlock(Position::tiles(5, 15));
    // if (tmp.isExpected()) {
    //     auto tmpBlock = tmp.value();
    //     tmpBlock.add<PlayerControlFree>();
    //     tmpBlock.add<Velocity>();
    // } else {
    //     print(tmp.error());
    // }

    for (s32 i = 21; i < 50; i++) {
        s32 y = 4;
        if (i < 26) {
            y = i - 19;
        } else if (i % 7 < 4) {
            continue;
        }
        createBlock(Transform2D::tiles(i, y),
                    Sprite(Depth::Player, *TextureManager::instance().getTextureAtlas(TEXNAME_SPRITE).getFrame("tile/dirtblock"), Colors::Magenta));
    }

    for (s32 i = 10; i < 15; i++) {
        Depth d = i % 2 == 0 ? Depth::Foreground1 : Depth::BackgroundNear;
        auto invisBlock =
            createBlock(Transform2D::tiles(i, 2),
                        Sprite(d, *TextureManager::instance().getTextureAtlas(TEXNAME_SPRITE).getFrame("tile/dirtblock"), Colors::Emerald))
                .value();
        invisBlock.remove<Collider>();
        auto invisBlock2 = createBlock(Transform2D::tiles(i - 5, 2), Draw(Colors::Emerald, {8, 8}, d)).value();
        invisBlock2.remove<Collider>();
    }

    auto platform = createBlock(Transform2D::tiles(5, 1)).value();
    auto pathControl = RailsControl(14,
                                    {
                                        {Transform2D::tiles(5, 1).position, RailsControl::Movement::LINEAR},
                                        {Transform2D::tiles(5, 15).position, RailsControl::Movement::EASEI_CUBE},
                                        // {Transform::tiles(15, 15).position, RailsControl::Movement::EASEI_CUBE},
                                    },
                                    2, true);
    platform.add<RailsControl>(pathControl);

    auto platformClone = createBlock(Transform2D::tiles(6, 1)).value();
    auto pathControlClone = RailsControl(
        14,
        {
            {Transform2D::tiles(6, 1).position, RailsControl::Movement::LINEAR}, {Transform2D::tiles(6, 15).position, RailsControl::Movement::LINEAR},
            // {Transform::tiles(5, 15).position, RailsControl::Movement::EASEIO_BEZIER},
            // {Transform::tiles(15, 15).position, RailsControl::Movement::EASEI_CUBE},
        },
        2, true);
    platformClone.add<RailsControl>(pathControlClone);

    auto rightPlatform = createBlock(Transform2D::tiles(36, 1)).value();
    rightPlatform.add(RailsControl(4,
                                   {
                                       {Transform2D::tiles(36, 1).position, RailsControl::Movement::LINEAR},
                                       {Transform2D::tiles(36, 7).position, RailsControl::Movement::LINEAR},
                                   },
                                   2));

    return Corrade::Containers::NullOpt;
}

void startRailsMovement(ecs::Entity self, ecs::Entity other, Collider* selfCollider, Collider* otherCollider, Vector2i moveNormal) {
    auto& rails = self.get<RailsControl>();
    if (rails.isWaiting && rails.curTarget == 0) {
        rails.startManually();
    }
}

void killEntityCallback(ecs::Entity self, ecs::Entity other, Collider* selfCollider, Collider* otherCollider, Vector2i moveNormal) {
    other.kill();
}

void createTestPlatform() {
    for (s32 x = 2; x < 6; x += 3) {
        auto trans = Transform2D::tiles(x, -15);
        auto platform = createBlock(trans).value();
        auto pathControl = RailsControl(112,
                                        {
                                            {Transform2D::tiles(x, -15).position, RailsControl::Movement::LINEAR},
                                            {Transform2D::tiles(x, -10).position, RailsControl::Movement::EASEI_CUBE},
                                        },
                                        2, false);
        platform.add<RailsControl>(pathControl);
        platform.add(Name("callback platform"));
        platform.get<Collider>().setCollisionCallback(&startRailsMovement);
        // platform.get<SolidCollider>().setCollisionCallback(&killEntityCallback);

        // platform.remove<SolidCollider>();
        // platform.add(SemiSolidCollider(trans, Vector2i(8, 8), Material::None, &startRailsMovement));

        // TESTING ATTACH COMPONENT
        // ------------------------
        auto frameOpt = TextureManager::instance().getTextureAtlas(TEXNAME_SPRITE).getFrame("tile/testgrass");
        if (frameOpt) {
            Sprite sprite = Sprite(Depth::Level, *frameOpt);
            Expected<ecs::Entity> grassOpt = createDecal(trans, sprite);
            if (grassOpt.isExpected()) {
                auto grass = grassOpt.value();
                grass.add(Attach(platform, {0, TEXELS_PER_TILE}));
            }
        }
    }
}

void createTestTrigger() {
    // TriggerCallback callback = [](ecs::Entity entity) { System::audio.play(Sfx::ENEMY_CRY); };
    // TriggerCallback callback = [](ecs::Entity self, ecs::Entity other) { other.kill(); };

    // TriggerZone trigger = TriggerZone(Transform::tiles(5, -5), {4, 4}, callback);
    // TriggerZone trigger = TriggerZone(Transform::tiles(5, -9), {4, 4}, nullptr);
    // TriggerZone trigger = TriggerZone(Transform2D::tiles(2, -9), {8, 8}, nullptr, callback);
    // auto newEntity = System::ecs->entity().value();
    // newEntity.add(trigger);
}

void createTestSemiSolid() {
    auto newEntity = System::world->entity().value();
    newEntity.add(Draw(Color(90, 127, 224, 255)));
    Transform2D trans = Transform2D::tiles(18, 10);
    // Transform2D trans = Transform2D::tiles(10, -14);
    // auto pathControl = RailsControl(64,
    //                                 {
    //                                     {Transform2D::tiles(10, -14).position, RailsControl::Movement::LINEAR},
    //                                     {Transform2D::tiles(10, -10).position, RailsControl::Movement::LINEAR},
    //                                 },
    //                                 2, true);
    // auto pathControl = RailsControl(64,
    //                                 {
    //                                     {Transform2D::tiles(10, -14).position, RailsControl::Movement::LINEAR},
    //                                     {Transform2D::tiles(12, -14).position, RailsControl::Movement::LINEAR},
    //                                 },
    //                                 1, true);
    newEntity.add(trans);
    // newEntity.add(pathControl);
    newEntity.add<Velocity>();
    newEntity.add<RigidBody>();
    auto collider = Collider::SemiSolid(trans, Vector2i(8, 8), WorldMaterial::None, nullptr);
    newEntity.add(collider);

    newEntity = System::world->entity().value();
    newEntity.add(Draw(Color(255, 127, 225, 255)));
    trans = Transform2D::tiles(18, 0);
    newEntity.add(trans);
    newEntity.add<Velocity>();
    newEntity.add<RigidBody>();
    newEntity.add(collider);
}

void createDepthTest() {
    auto newEntity = System::world->entity().value();
    newEntity.add(Draw(Color(56, 56, 255, 255), {8, 8}, Depth::BackgroundNear));
    newEntity.add(Transform2D::tiles(7, -14));

    newEntity = System::world->entity().value();
    newEntity.add(Draw(Color(56, 56, 200, 255), {8, 8}, Depth::Foreground1));
    newEntity.add(Transform2D::tiles(8, -14));
}

void createTestMouseTracker() {
    // auto newEntity = System::ecs->entity().value();
    // newEntity.add<Transform>();
    // newEntity.add(Draw(Color(56, 127, 150, 255), {8, 8}, Depth::Debug));
    //
    // auto setPositionToCamera = [](ecs::Entity entity) { entity.set(Transform(screenToWorldCoords(System::input.MousePosition))); };
    // newEntity.add(OnFrameEnd(setPositionToCamera, false));
}

void createPaletteTest() {
    auto newEntity = System::world->entity().value();
    // newEntity.add(Draw(Color(56, 56, 255, 255), {16, 4}, Depth::Level));
    auto frame = TextureManager::instance().getTextureAtlas(TEXNAME_SPRITE).getFrame("actor/palette");
    if (frame) {
        auto sprite = Sprite(Depth::Level, *frame);
        sprite.scale = {1, 1};
        newEntity.add(sprite);
        newEntity.add(Transform2D::tiles(7, -8));

    } else {
        print("couldn't find frame");
    }
}

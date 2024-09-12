#include "ComponentFactory.h"

#include "CorradeOptional.h"
#include "Game/Entities/Animations.h"
#include "json.hpp"

#include "Game/Components/Switch.h"
#include "Game/Entities/Checkpoint.h"
#include "Util/Vector.h"
#include "whalECS/src/ECS.h"

#include "Gfx/Texture.h"
#include "Physics/CollisionLayer.h"
#include "Settings.h"

#include "Map/Level.h"
#include "Map/Tiled.h"

#include "Components/Animator.h"
#include "Components/Collision.h"
#include "Components/Draw.h"
#include "Components/Lifetime.h"
#include "Components/Light.h"
#include "Components/Name.h"
#include "Components/ParticleEmitter.h"
#include "Components/PlayerControl.h"
#include "Components/RailsControl.h"
#include "Components/RigidBody.h"
#include "Components/Tags.h"
#include "Components/Transform.h"
#include "Components/TriggerZone.h"
#include "Components/Velocity.h"
#include "Systems/TagTrackers.h"

#include "Util/Print.h"

#ifndef NDEBUG
#define MY_ASSERT(cond, msg)                                                                                                                         \
    do {                                                                                                                                             \
        if (!(cond)) {                                                                                                                               \
            std::ostringstream str;                                                                                                                  \
            str << msg;                                                                                                                              \
            std::cerr << str.str() << std::endl;                                                                                                     \
            std::abort();                                                                                                                            \
        }                                                                                                                                            \
    } while (0)
#else
#define MY_ASSERT(cond, msg)                                                                                                                         \
    do {                                                                                                                                             \
    } while (0)
#endif

namespace whal {

static const char* KEY_MEMBERS = "members";
static const char* KEY_NAME = "name";
static const char* KEY_VALUE = "value";

template <typename T>
T readVal(const nlohmann::json& object, std::string_view key);

template <typename T>
bool tryReadVal(const nlohmann::json& object, std::string_view key, T* dst);

static void addTagComponents(const nlohmann::json& values, const nlohmann::json& allObjects,
                             const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData,
                             const ActiveLevel& level, ecs::Entity entity, LayerData layerData);
static void addComponentVelocity(const nlohmann::json& values, const nlohmann::json& allObjects,
                                 const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData,
                                 const ActiveLevel& level, ecs::Entity entity, LayerData layerData);
static void addComponentRailsControl(const nlohmann::json& values, const nlohmann::json& allObjects,
                                     const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData,
                                     const ActiveLevel& level, ecs::Entity entity, LayerData layerData);
static void addComponentCollider(const nlohmann::json& values, const nlohmann::json& allObjects,
                                 const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData,
                                 const ActiveLevel& level, ecs::Entity entity, LayerData layerData);
static void addComponentTrigger(const nlohmann::json& values, const nlohmann::json& allObjects,
                                const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData,
                                const ActiveLevel& level, ecs::Entity entity, LayerData layerData);
static void addComponentRigidBody(const nlohmann::json& values, const nlohmann::json& allObjects,
                                  const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData,
                                  const ActiveLevel& level, ecs::Entity entity, LayerData layerData);
static void addComponentPlayerControl(const nlohmann::json& values, const nlohmann::json& allObjects,
                                      const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData,
                                      const ActiveLevel& level, ecs::Entity entity, LayerData layerData);
static void addComponentJumper(const nlohmann::json& values, const nlohmann::json& allObjects,
                               const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData,
                               const ActiveLevel& level, ecs::Entity entity, LayerData layerData);
static void addComponentDraw(const nlohmann::json& values, const nlohmann::json& allObjects,
                             const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData,
                             const ActiveLevel& level, ecs::Entity entity, LayerData layerData);
static void addComponentSprite(const nlohmann::json& values, const nlohmann::json& allObjects,
                               const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData,
                               const ActiveLevel& level, ecs::Entity entity, LayerData layerData);
static void addComponentAnimator(const nlohmann::json& values, const nlohmann::json& allObjects,
                                 const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData,
                                 const ActiveLevel& level, ecs::Entity entity, LayerData layerData);
static void addDrawLayer(const nlohmann::json& values, const nlohmann::json& allObjects,
                         const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, const ActiveLevel& level,
                         ecs::Entity entity, LayerData layerData);
static void addComponentFadeout(const nlohmann::json& values, const nlohmann::json& allObjects,
                                const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData,
                                const ActiveLevel& level, ecs::Entity entity, LayerData layerData);
static void addComponentLight(const nlohmann::json& values, const nlohmann::json& allObjects,
                              const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData,
                              const ActiveLevel& level, ecs::Entity entity, LayerData layerData);
static void addComponentRadiance(const nlohmann::json& values, const nlohmann::json& allObjects,
                                 const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData,
                                 const ActiveLevel& level, ecs::Entity entity, LayerData layerData);
static void addComponentLifetime(const nlohmann::json& values, const nlohmann::json& allObjects,
                                 const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData,
                                 const ActiveLevel& level, ecs::Entity entity, LayerData layerData);
static void addComponentFollow(const nlohmann::json& values, const nlohmann::json& allObjects,
                               const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData,
                               const ActiveLevel& level, ecs::Entity entity, LayerData layerData);
static void addComponentAttach(const nlohmann::json& values, const nlohmann::json& allObjects,
                               const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData,
                               const ActiveLevel& level, ecs::Entity entity, LayerData layerData);
static void addComponentOrbit(const nlohmann::json& values, const nlohmann::json& allObjects,
                              const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData,
                              const ActiveLevel& level, ecs::Entity entity, LayerData layerData);

static bool loadCheckpoints(const nlohmann::json& checkpointData, std::vector<RailsControl::CheckPoint>& dstCheckpoints, const ActiveLevel& level);

static void addSwitchComponent(const nlohmann::json& values, const nlohmann::json& allObjects,
                               const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData,
                               const ActiveLevel& level, ecs::Entity entity, LayerData layerData);

static void addSwitchGateComponent(const nlohmann::json& values, const nlohmann::json& allObjects,
                                   const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData,
                                   const ActiveLevel& level, ecs::Entity entity, LayerData layerData);

static void addComponentText(const nlohmann::json& values, const nlohmann::json& allObjects,
                             const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData,
                             const ActiveLevel& level, ecs::Entity entity, LayerData layerData);

static void addComponentParticleEmitter(const nlohmann::json& values, const nlohmann::json& allObjects,
                                        const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData,
                                        const ActiveLevel& level, ecs::Entity entity, LayerData layerData);

static Velocity DefaultVelocity;
static RailsControl DefaultRailsControl;
static Collider DefaultCollider;
static Trigger DefaultTrigger;
static RigidBody DefaultRigidBody;
static PlayerControl DefaultPlayerControl;
static Jumper DefaultJumper;
static DrawRect DefaultDraw;
static Sprite DefaultSprite;
static Sprite DefaultAnimatedSprite;
static FadeOut DefaultFadeout;
static Follow DefaultFollow;
static Attach DefaultAttach;
static PointLight DefaultPointLight;
static Radiance DefaultRadiance;
static Lifetime DefaultLifeTime;
static SwitchGate DefaultSwitchGate;
static DrawText DefaultDrawText;
static ParticleEmitter DefaultParticleEmitter;
static Orbit DefaultOrbit;

static NameToCreator<ComponentAdder> S_COMPONENT_ENTRIES[] = {
    {"Component_RailsControl", addComponentRailsControl},
    // {"Animator", addComponentAnimator},
    {"Component_Collider", addComponentCollider},
    {"Component_Trigger", addComponentTrigger},
    {"Component_Draw", addComponentDraw},
    {"Component_Sprite_NoAnim", addComponentSprite},
    {"Component_Sprite_Animated", addComponentAnimator},
    {"Component_DrawLayer_TileOnly", addDrawLayer},
    {"Component_FadeOut", addComponentFadeout},
    {"Component_PointLight", addComponentLight},
    {"Component_Radiance", addComponentRadiance},
    {"Component_Lifetime", addComponentLifetime},
    {"Component_Follow", addComponentFollow},
    {"Component_Attach", addComponentAttach},
    {"Component_RigidBody", addComponentRigidBody},
    {"Component_PlayerControl", addComponentPlayerControl},
    {"Component_Jumper", addComponentJumper},
    {"Component_Velocity", addComponentVelocity},
    {"Component_Tags", addTagComponents},
    {"Component_Switch", addSwitchComponent},
    {"Component_SwitchGate", addSwitchGateComponent},
    {"Component_Text", addComponentText},
    {"Component_ParticleEmitter", addComponentParticleEmitter},
    {"Component_Orbit", addComponentOrbit},
};

ComponentFactory::ComponentFactory() : Factory<ComponentAdder>("ComponentFactory") {
    initFactory(S_COMPONENT_ENTRIES);
}

void ComponentFactory::makeDefaultComponent(const nlohmann::json& property) {
    std::string componentName = property[KEY_NAME];
    ComponentAdder creatorFunc = nullptr;
    if (getEntryIndex(componentName.c_str(), &creatorFunc) == -1) {
        return;
    }

    if (componentName == "Component_RailsControl") {
        DefaultRailsControl = RailsControl();
        for (const auto& member : property[KEY_MEMBERS]) {
            std::string memberName = member[KEY_NAME];
            if (memberName == "RailsCycleBehavior") {
                // do nothing; default hard coded in factory method b/c it's not 1:1 with struct data
            } else if (memberName == "speed") {
                DefaultRailsControl.speed = member[KEY_VALUE];
            } else if (memberName == "waitTime") {
                DefaultRailsControl.waitTime = member[KEY_VALUE];
            } else if (memberName == "Checkpoints") {
                // do nothing
            } else {
                print("Skipping member ", memberName, "for", componentName);
            }
        }

    } else if (componentName == "Component_Collider") {
        DefaultCollider = Collider();
        for (const auto& member : property[KEY_MEMBERS]) {
            std::string memberName = member[KEY_NAME];
            if (memberName == "CollisionDir") {
                DefaultCollider.setCollisionDir(member[KEY_VALUE]);
            } else if (memberName == "Material") {
                DefaultCollider.setMaterial(member[KEY_VALUE]);
            } else if (memberName == "Layer") {
                std::string layer = member[KEY_VALUE];
                DefaultCollider.setCollisionLayer(CollisionLayer::fromString(layer.c_str()));
            } else if (memberName == "Shape") {
                // do nothing
            } else {
                print("Skipping member ", memberName, "for", componentName);
            }
        }

    } else if (componentName == "Component_Trigger") {
        DefaultTrigger = Trigger();
        for (const auto& member : property[KEY_MEMBERS]) {
            std::string memberName = member[KEY_NAME];
            if (memberName == "Layer") {
                std::string layer = member[KEY_VALUE];
                DefaultTrigger.layer = CollisionLayer::fromString(layer.c_str());
            } else if (memberName == "Shape") {
                // do nothing
            } else {
                print("Skipping member ", memberName, "for", componentName);
            }
        }

    } else if (componentName == "Component_Draw") {
        DefaultDraw = DrawRect();
        for (const auto& member : property[KEY_MEMBERS]) {
            std::string memberName = member[KEY_NAME];
            if (memberName == "Color") {
                std::string hexString = member[KEY_VALUE];
                DefaultDraw.setColor(hexStringARGBToColor(hexString));
            } else if (memberName == "Layer") {
                // do nothing
            } else {
                print("Skipping member ", memberName, "for", componentName);
            }
        }

    } else if (componentName == "Component_Sprite_NoAnim") {
        DefaultSprite = Sprite();
        for (const auto& member : property[KEY_MEMBERS]) {
            std::string memberName = member[KEY_NAME];
            if (memberName == "Color") {
                std::string hexString = member[KEY_VALUE];
                DefaultSprite.setColor(hexStringARGBToColor(hexString));
            } else if (memberName == "Sprite") {
                // do nothing
            } else if (memberName == "rotationDegrees") {
                // do nothing, affects Transform
            } else if (memberName == "rotateAboutCenter") {
                DefaultSprite.isRotateAboutCenter = member[KEY_VALUE];
            } else {
                print("Skipping member ", memberName, "for", componentName);
            }
        }

    } else if (componentName == "Component_Sprite_Animated") {
        DefaultAnimatedSprite = Sprite();
        for (const auto& member : property[KEY_MEMBERS]) {
            std::string memberName = member[KEY_NAME];
            if (memberName == "Color") {
                std::string hexString = member[KEY_VALUE];
                DefaultAnimatedSprite.setColor(hexStringARGBToColor(hexString));
            } else if (memberName == "Sprite") {
                // do nothing
            } else if (memberName == "rotationDegrees") {
                // do nothing, affects Transform
            } else if (memberName == "rotateAboutCenter") {
                DefaultAnimatedSprite.isRotateAboutCenter = member[KEY_VALUE];
            } else {
                print("Skipping member ", memberName, "for", componentName);
            }
        }

    } else if (componentName == "Component_PointLight") {
        DefaultPointLight = PointLight();
        for (const auto& member : property[KEY_MEMBERS]) {
            std::string memberName = member[KEY_NAME];
            if (memberName == "Color") {
                std::string hexString = member[KEY_VALUE];
                DefaultPointLight.color = hexStringARGBToColor(hexString);
            } else if (memberName == "heightTexels") {
                DefaultPointLight.heightTexels = member[KEY_VALUE];
            } else if (memberName == "radiusTexels") {
                DefaultPointLight.radiusTexels = member[KEY_VALUE];
            } else {
                print("Skipping member ", memberName, "for", componentName);
            }
        }

    } else if (componentName == "Component_Radiance") {
        DefaultRadiance = Radiance();
        for (const auto& member : property[KEY_MEMBERS]) {
            std::string memberName = member[KEY_NAME];
            if (memberName == "Color") {
                std::string hexString = member[KEY_VALUE];
                DefaultRadiance.color = hexStringARGBToColor(hexString);
            } else if (memberName == "heightTexels") {
                DefaultRadiance.heightTexels = member[KEY_VALUE];
            } else if (memberName == "radiusTexels") {
                DefaultRadiance.radiusTexels = member[KEY_VALUE];
            } else {
                print("Skipping member ", memberName, "for", componentName);
            }
        }

    } else if (componentName == "Component_Lifetime") {
        DefaultLifeTime = Lifetime();
        for (const auto& member : property[KEY_MEMBERS]) {
            std::string memberName = member[KEY_NAME];
            if (memberName == "seconds") {
                DefaultLifeTime.secondsRemaining = member[KEY_VALUE];
            } else {
                print("Skipping member ", memberName, "for", componentName);
            }
        }

    } else if (componentName == "Component_FadeOut") {
        DefaultFadeout = FadeOut();
        for (const auto& member : property[KEY_MEMBERS]) {
            std::string memberName = member[KEY_NAME];
            if (memberName == "seconds") {
                DefaultFadeout.time = member[KEY_VALUE];
            } else if (memberName == "startAlpha") {
                DefaultFadeout.startAlpha = member[KEY_VALUE];
            } else if (memberName == "endAlpha") {
                DefaultFadeout.endAlpha = member[KEY_VALUE];
            } else {
                print("Skipping member ", memberName, "for", componentName);
            }
        }

    } else if (componentName == "Component_RigidBody") {
        DefaultRigidBody = RigidBody();
        for (const auto& member : property[KEY_MEMBERS]) {
            std::string memberName = member[KEY_NAME];
            if (memberName == "momentumMultiplierX") {
                DefaultRigidBody.momentumMultiplier.x = member[KEY_VALUE];
            } else if (memberName == "momentumMultiplierY") {
                DefaultRigidBody.momentumMultiplier.y = member[KEY_VALUE];
            } else if (memberName == "frictionGround") {
                DefaultRigidBody.frictionMultiplier.x = member[KEY_VALUE];
            } else if (memberName == "frictionAir") {
                DefaultRigidBody.frictionMultiplier.y = member[KEY_VALUE];
            } else if (memberName == "gravityMultiplier") {
                DefaultRigidBody.gravityMultiplier = member[KEY_VALUE];
            } else {
                print("Skipping member ", memberName, "for", componentName);
            }
        }

    } else if (componentName == "Component_Jumper") {
        DefaultJumper = Jumper();
        for (const auto& member : property[KEY_MEMBERS]) {
            std::string memberName = member[KEY_NAME];
            if (memberName == "coyoteTimeSecondsMax") {
                DefaultJumper.coyoteTimeSecondsMax = member[KEY_VALUE];
            } else if (memberName == "jumpInitialVelocity") {
                DefaultJumper.jumpInitialVelocity = member[KEY_VALUE];
            } else if (memberName == "jumpSecondsMax") {
                DefaultJumper.jumpSecondsMax = member[KEY_VALUE];
            } else {
                print("Skipping member ", memberName, "for", componentName);
            }
        }

    } else if (componentName == "Component_Velocity") {
        DefaultVelocity = Velocity();
        for (const auto& member : property[KEY_MEMBERS]) {
            std::string memberName = member[KEY_NAME];
            if (memberName == "velX") {
                DefaultVelocity.stable.x = member[KEY_VALUE];
            } else if (memberName == "velY") {
                DefaultVelocity.stable.y = member[KEY_VALUE];
            } else {
                print("Skipping member ", memberName, "for", componentName);
            }
        }
    } else if (componentName == "Component_Follow") {
        DefaultFollow = Follow();
        for (const auto& member : property[KEY_MEMBERS]) {
            std::string memberName = member[KEY_NAME];
            if (memberName == "lookAheadX") {
                DefaultFollow.lookAheadTexels.x = member[KEY_VALUE];
            } else if (memberName == "lookAheadY") {
                DefaultFollow.lookAheadTexels.y = member[KEY_VALUE];
            } else if (memberName == "deadZoneX") {
                DefaultFollow.deadZoneTexels.x = member[KEY_VALUE];
            } else if (memberName == "deadZoneY") {
                DefaultFollow.deadZoneTexels.y = member[KEY_VALUE];
            } else if (memberName == "dampingX") {
                DefaultFollow.damping.x = member[KEY_VALUE];
            } else if (memberName == "dampingY") {
                DefaultFollow.damping.y = member[KEY_VALUE];
            } else if (memberName == "FollowTarget") {
                // do nothing
            } else if (memberName == "boundsHalflenX") {
                DefaultFollow.boundsXTexels = Vector2i(member[KEY_VALUE], member[KEY_VALUE]);
            } else if (memberName == "boundsHalflenY") {
                DefaultFollow.boundsYTexels = Vector2i(member[KEY_VALUE], member[KEY_VALUE]);
            } else {
                print("Skipping member ", memberName, "for", componentName);
            }
        }

    } else if (componentName == "Component_Text") {
        DefaultDrawText = DrawText();
        for (const auto& member : property[KEY_MEMBERS]) {
            std::string memberName = member[KEY_NAME];
            if (memberName == "text") {
                DefaultDrawText.text = member[KEY_VALUE];
            } else if (memberName == "color") {
                std::string hexString = member[KEY_VALUE];
                DefaultDrawText.color = hexStringARGBToColor(hexString);
            } else if (memberName == "center") {
                DefaultDrawText.isCentered = member[KEY_VALUE];
            } else {
                print("Skipping member ", memberName, "for", componentName);
            }
        }

    } else if (componentName == "Component_ParticleEmitter") {
        DefaultParticleEmitter = ParticleEmitter();
        for (const auto& member : property[KEY_MEMBERS]) {
            std::string memberName = member[KEY_NAME];
            if (memberName == "maxSpeed") {
                DefaultParticleEmitter.maxSpeedTexelsPerSecond = member[KEY_VALUE];

            } else if (memberName == "particlesPerSecond") {
                DefaultParticleEmitter.particlesPerSecond = member[KEY_VALUE];

            } else if (memberName == "Depth") {
                DefaultParticleEmitter.depth = member[KEY_VALUE];

            } else if (memberName == "LifetimeMultiplier") {
                DefaultParticleEmitter.lifetimeMultiplier = member[KEY_VALUE];

            } else if (memberName == "Direction") {
                CollisionDir dir = member[KEY_VALUE];
                DefaultParticleEmitter.setDirection(dir);

            } else if (memberName == "Material") {
                WorldMaterial material = member[KEY_VALUE];
                DefaultParticleEmitter.material = material;

            } else if (memberName == "Shape") {
                // do nothing

            } else {
                print("Skipping member ", memberName, "for", componentName);
            }
        }

    } else if (componentName == "Component_Orbit") {
        DefaultOrbit = Orbit();

        for (const auto& member : property[KEY_MEMBERS]) {
            std::string memberName = member[KEY_NAME];
            if (memberName == "Radius") {
                DefaultOrbit.radius = member[KEY_VALUE];
            } else if (memberName == "RotationsPerSecond") {
                DefaultOrbit.rotationsPerSecond = member[KEY_VALUE];
            } else if (memberName == "Offset") {
                // do nothing
            }
        }

    } else if (componentName == "Component_SwitchGate") {
        DefaultSwitchGate = SwitchGate();
        for (const auto& member : property[KEY_MEMBERS]) {
            std::string memberName = member[KEY_NAME];
            if (memberName == "numKeys") {
                DefaultSwitchGate.numKeys = member[KEY_VALUE];
            } else if (memberName == "isPersistent") {
                DefaultSwitchGate.isPersistent = member[KEY_VALUE];
            } else {
                print("Skipping member ", memberName, "for", componentName);
            }
        }
    } else if (componentName == "Component_Switch") {
        // do nothing
    } else {
        print("unhandled default component type: ", componentName);
    }
}

void addComponentVelocity(const nlohmann::json& values, const nlohmann::json& allObjects,
                          const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, const ActiveLevel& level,
                          ecs::Entity entity, LayerData layerData) {
    Velocity velocity = entity.has<Velocity>() ? entity.get<Velocity>() : DefaultVelocity;
    tryReadVector2f(values, "velX", "velY", &velocity.stable);

    entity.add(velocity);
}

void addComponentRailsControl(const nlohmann::json& values, const nlohmann::json& allObjects,
                              const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData,
                              const ActiveLevel& level, ecs::Entity entity, LayerData layerData) {
    std::vector<RailsControl::CheckPoint> checkpoints;
    bool isCycle = false;
    if (values.contains("Checkpoints")) {
        s32 id = values["Checkpoints"];
        const nlohmann::json checkPointObj = allObjects.at(idToIndex.at(id).first);
        isCycle = loadCheckpoints(checkPointObj, checkpoints, level);
    }

    RailsControl rails = entity.has<RailsControl>() ? entity.get<RailsControl>() : DefaultRailsControl;
    rails.setCheckpoints(checkpoints, entity.get<Transform2D>());

    std::string cycleBehavior = "ManualStart";
    tryReadString(values, "CycleBehavior", &cycleBehavior);
    if (isCycle) {
        if (cycleBehavior == "Automatic") {
            rails.endBehavior = RailsControl::CycleBehavior::AUTOMATIC_LOOP;
        } else if (cycleBehavior == "ManualStart") {
            rails.endBehavior = RailsControl::CycleBehavior::MANUAL_FIRSTSTEP_LOOP;
        } else {
            rails.endBehavior = RailsControl::CycleBehavior::MANUAL_ALLSTEPS_LOOP;
        }
    } else {
        if (cycleBehavior == "Automatic") {
            rails.endBehavior = RailsControl::CycleBehavior::AUTOMATIC_BACKTRACK;
        } else if (cycleBehavior == "ManualStart") {
            rails.endBehavior = RailsControl::CycleBehavior::MANUAL_FIRSTSTEP_BACKTRACK;
        } else {
            rails.endBehavior = RailsControl::CycleBehavior::MANUAL_ALLSTEPS_BACKTRACK;
        }
    }

    tryReadFloat(values, "speed", &rails.speed);
    tryReadFloat(values, "waitTime", &rails.waitTime);

    entity.add(rails);
}

static u32 parseGfxEffects(const nlohmann::json& values) {
    u32 flags = 0;
    if (values.contains("Layer")) {
        std::string textureLayer = values["Layer"];
        if (textureLayer == "Bloom") {
            flags |= PostProcessFlag::Bloom;
        } else if (textureLayer == "Glow") {
            flags |= PostProcessFlag::Glow;
        }
    }
    return flags;
}

void addComponentDraw(const nlohmann::json& values, const nlohmann::json& allObjects,
                      const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, const ActiveLevel& level,
                      ecs::Entity entity, LayerData layerData) {
    DrawRect draw = entity.has<Draw>() ? entity.get<Draw>().getRect() : DefaultDraw;
    draw.depth = layerData.depth;
    draw.setFrameSize(entityData.dimensionsTexels);

    // ARGB
    if (values.contains("Color")) {
        std::string hexcode = "#ffffffff";
        hexcode = values["Color"];
        Color color = hexStringARGBToColor(hexcode);
        draw.setColor(color);
    }

    u32 flags = parseGfxEffects(values);
    entity.add(Draw(draw, flags));
}

void addComponentSprite(const nlohmann::json& values, const nlohmann::json& allObjects,
                        const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, const ActiveLevel& level,
                        ecs::Entity entity, LayerData layerData) {
    Sprite sprite = entity.has<Draw>() ? entity.get<Draw>().getSprite() : DefaultSprite;

    s32 rotationDegrees;
    if (tryReadInt(values, "rotationDegrees", &rotationDegrees)) {
        entity.get<Transform2D>().rotationDegrees = rotationDegrees;
    }

    tryReadBool(values, "rotateAboutCenter", &sprite.isRotateAboutCenter);

    // ARGB
    if (values.contains("Color")) {
        std::string hexcode = "#ffffffff";
        hexcode = values["Color"];
        Color color = hexStringARGBToColor(hexcode);
        sprite.setColor(color);
    }

    u32 flags = parseGfxEffects(values);

    std::string spritePath = "";
    if (values.contains("Sprite")) {
        spritePath = values["Sprite"];
        std::replace(spritePath.begin(), spritePath.end(), '\\', '/');
    }
    auto frameOpt = TextureManager::instance().getTextureAtlas(TEXNAME_SPRITE).getFrame(spritePath.c_str());
    if (frameOpt) {
        sprite.depth = layerData.depth;
        sprite.setFrame(*frameOpt);
        entity.add(Draw(sprite, flags));
    } else {
        print("Coudn't find frame for sprite:", spritePath);
        // add draw instead
        DrawRect draw = DefaultDraw;
        draw.setFrameSize(entityData.dimensionsTexels);
        entity.add(Draw(draw, flags));
    }
}

void addComponentAnimator(const nlohmann::json& values, const nlohmann::json& allObjects,
                          const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, const ActiveLevel& level,
                          ecs::Entity entity, LayerData layerData) {
    Sprite sprite = entity.has<Draw>() ? entity.get<Draw>().getSprite() : DefaultAnimatedSprite;
    std::string animatorName = readString(values, "Animator");
    Animator animator = getAnimator(animatorName.c_str());
    entity.add(animator);
    sprite.setFrame(animator.getFrame());

    s32 rotationDegrees;
    if (tryReadInt(values, "rotationDegrees", &rotationDegrees)) {
        entity.get<Transform2D>().rotationDegrees = rotationDegrees;
    }

    tryReadBool(values, "rotateAboutCenter", &sprite.isRotateAboutCenter);

    // ARGB
    if (values.contains("Color")) {
        std::string hexcode = "#ffffffff";
        hexcode = values["Color"];
        Color color = hexStringARGBToColor(hexcode);
        sprite.setColor(color);
    }

    u32 flags = parseGfxEffects(values);

    sprite.depth = layerData.depth;
    entity.add(Draw(sprite, flags));
}

// this is only intended to be used on tiles (which already have a sprite), not objects
void addDrawLayer(const nlohmann::json& values, const nlohmann::json& allObjects,
                  const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, const ActiveLevel& level,
                  ecs::Entity entity, LayerData layerData) {
    if (!entity.has<Draw>()) {
        print("can't add draw layer for entity", entityData.id, "without Draw component");
        return;
    }

    u32 flags = parseGfxEffects(values);
    entity.get<Draw>().setPostProcessFlags(flags);
}

void addComponentFadeout(const nlohmann::json& values, const nlohmann::json& allObjects,
                         const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, const ActiveLevel& level,
                         ecs::Entity entity, LayerData layerData) {
    FadeOut fadeout = entity.has<FadeOut>() ? entity.get<FadeOut>() : DefaultFadeout;
    tryReadFloat(values, "seconds", &fadeout.time);
    tryReadFloat(values, "startAlpha", &fadeout.startAlpha);
    tryReadFloat(values, "endAlpha", &fadeout.endAlpha);

    entity.add(fadeout);
}

void addComponentLight(const nlohmann::json& values, const nlohmann::json& allObjects,
                       const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, const ActiveLevel& level,
                       ecs::Entity entity, LayerData layerData) {
    PointLight light = entity.has<PointLight>() ? entity.get<PointLight>() : DefaultPointLight;
    if (!tryReadVal(values, "radiusTexels", &light.radiusTexels)) {
        // by default, use bigger dimension
        light.radiusTexels = std::max(entityData.dimensionsTexels.x, entityData.dimensionsTexels.y);
    }
    if (!tryReadVal(values, "heightTexels", &light.heightTexels)) {
        // by default, use half of entity height
        light.heightTexels = entityData.dimensionsTexels.y / 2;
    }
    std::string hexString;
    if (tryReadVal(values, "Color", &hexString)) {
        light.color = hexStringARGBToColor(hexString);
    }
    entity.add(light);
}

void addComponentRadiance(const nlohmann::json& values, const nlohmann::json& allObjects,
                          const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, const ActiveLevel& level,
                          ecs::Entity entity, LayerData layerData) {
    Radiance light = entity.has<Radiance>() ? entity.get<Radiance>() : DefaultRadiance;
    if (!tryReadVal(values, "radiusTexels", &light.radiusTexels)) {
        // by default, use bigger dimension
        light.radiusTexels = std::max(entityData.dimensionsTexels.x, entityData.dimensionsTexels.y);
    }
    if (!tryReadVal(values, "heightTexels", &light.heightTexels)) {
        // by default, use half of entity height
        light.heightTexels = entityData.dimensionsTexels.y / 2;
    }
    std::string hexString;
    if (tryReadVal(values, "Color", &hexString)) {
        light.color = hexStringARGBToColor(hexString);
    }
    entity.add(light);
}

void addComponentLifetime(const nlohmann::json& values, const nlohmann::json& allObjects,
                          const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, const ActiveLevel& level,
                          ecs::Entity entity, LayerData layerData) {
    Lifetime lifetime = entity.has<Lifetime>() ? entity.get<Lifetime>() : DefaultLifeTime;
    tryReadVal(values, "seconds", &lifetime.secondsRemaining);
    entity.add(lifetime);
}

void addComponentCollider(const nlohmann::json& values, const nlohmann::json& allObjects,
                          const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, const ActiveLevel& level,
                          ecs::Entity entity, LayerData layerData) {
    Collider collider = entity.has<Collider>() ? entity.get<Collider>() : DefaultCollider;
    CollisionDir collisionDir = collider.getCollisionDir();
    WorldMaterial material = collider.getMaterial();

    if (tryReadVal(values, "CollisionDir", &collisionDir)) {
        collider.setCollisionDir(collisionDir);
    }
    if (tryReadVal(values, "Material", &material)) {
        collider.setMaterial(material);
    }

    std::string layerName;
    if (tryReadVal(values, "Layer", &layerName)) {
        collider.setCollisionLayer(CollisionLayer::fromString(layerName.c_str()));
    }

    if (values.contains("Shape")) {
        s32 shapeId = readInt(values, "Shape");
        // calc distance between this object and Shape for the offset
        const auto& shapeObj = allObjects[idToIndex.at(shapeId).first];
        const Vector2i otherDimsTexels = readVector2i(shapeObj, "width", "height");
        const Vector2i halflenTexels = otherDimsTexels / 2;
        const Vector2i thisTrans = getTransformFromMapPosition(entityData.position, entityData.dimensionsTexels, level, entityData.isPoint).position;

        const Vector2i otherTrans = getTransformFromMapPosition(readVector2i(shapeObj), otherDimsTexels, level, false).position;

        const auto offset = otherTrans - thisTrans;
        Transform2D transOffset = entity.get<Transform2D>();
        if (!offset.isZero()) {
            entity.add(ColliderOffset(offset));
            transOffset.position += offset;
        }
        collider.setShape(AABB(transOffset, halflenTexels * PIXELS_PER_TEXEL));
    } else {
        // there's no default shape object. Instead use the entity's dimensions
        collider.setShape(AABB(entity.get<Transform2D>(), entityData.dimensionsTexels * PIXELS_PER_TEXEL / 2));
    }
    entity.add(collider);
}

void addComponentTrigger(const nlohmann::json& values, const nlohmann::json& allObjects,
                         const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, const ActiveLevel& level,
                         ecs::Entity entity, LayerData layerData) {
    Trigger trigger = entity.has<Trigger>() ? entity.get<Trigger>() : DefaultTrigger;

    std::string layerName;
    if (tryReadVal(values, "Layer", &layerName)) {
        trigger.layer = CollisionLayer::fromString(layerName.c_str());
    }

    if (values.contains("Shape")) {
        s32 shapeId = readInt(values, "Shape");
        // calc distance between this object and Shape for the offset
        const auto& shapeObj = allObjects[idToIndex.at(shapeId).first];
        const Vector2i otherDimsTexels = readVector2i(shapeObj, "width", "height");
        const Vector2i halflenTexels = otherDimsTexels / 2;
        const Vector2i thisTrans = getTransformFromMapPosition(entityData.position, entityData.dimensionsTexels, level, entityData.isPoint).position;

        const Vector2i otherTrans = getTransformFromMapPosition(readVector2i(shapeObj), otherDimsTexels, level, false).position;

        trigger.offset = otherTrans - thisTrans;

        if (shapeObj.contains("ellipse")) {
            const s32 radius = std::max(halflenTexels.x, halflenTexels.y) * PIXELS_PER_TEXEL;
            trigger.shape = Circle(entity.get<Transform2D>().position + trigger.offset + Vector2i(0, halflenTexels.y * PIXELS_PER_TEXEL), radius);

        } else {
            trigger.shape = AABB(entity.get<Transform2D>().position + trigger.offset + Vector2i(0, halflenTexels.y * PIXELS_PER_TEXEL),
                                 halflenTexels * PIXELS_PER_TEXEL);
        }
    } else {
        if (allObjects[idToIndex.at(entityData.id).first].contains("ellipse")) {
            const s32 radius = std::max(entityData.dimensionsTexels.x, entityData.dimensionsTexels.y) * PIXELS_PER_TEXEL / 2;
            trigger.shape = Circle(entity.get<Transform2D>(), radius);

        } else {
            trigger.shape = AABB(entity.get<Transform2D>(), entityData.dimensionsTexels * PIXELS_PER_TEXEL / 2);
        }
    }

    entity.add(trigger);
}

void addComponentFollow(const nlohmann::json& values, const nlohmann::json& allObjects,
                        const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, const ActiveLevel& level,
                        ecs::Entity entity, LayerData layerData) {
    entity.add(loadFollowComponent(values, level));
}

void addComponentAttach(const nlohmann::json& values, const nlohmann::json& allObjects,
                        const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, const ActiveLevel& level,
                        ecs::Entity entity, LayerData layerData) {
    Attach attach = entity.has<Attach>() ? entity.get<Attach>() : DefaultAttach;

    s32 targetId;
    if (!tryReadInt(values, "target", &targetId)) {
        print("Entity with Map id ", entityData.id, "has attach component with no target");
        return;
    }

    tryReadVal(values, "DirectionParam", &attach.directionParam);

    ecs::Entity target = idToIndex.at(targetId).second;
    attach.targetEntityID = target.id();

    auto thisPosition = entity.get<Transform2D>().position;

    // other isn't guaranteed to have been parsed. Calculate its transform manually
    const auto& targetObj = allObjects[idToIndex.at(targetId).first];
    Vector2i otherDimsTexels = getObjectSize(targetObj);
    const Vector2i otherPosition = getTransformFromMapPosition(readVector2i(targetObj), otherDimsTexels, level, false).position;

    attach.offsetTexels = (thisPosition - otherPosition) / PIXELS_PER_TEXEL;
    entity.add(attach);
}

void addComponentRigidBody(const nlohmann::json& values, const nlohmann::json& allObjects,
                           const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, const ActiveLevel& level,
                           ecs::Entity entity, LayerData layerData) {
    RigidBody rb = entity.has<RigidBody>() ? entity.get<RigidBody>() : DefaultRigidBody;

    tryReadVector2f(values, "momentumMultiplierX", "momentumMultiplierY", &rb.momentumMultiplier);
    tryReadVector2f(values, "frictionGround", "frictionAir", &rb.frictionMultiplier);
    tryReadFloat(values, "gravityMultiplier", &rb.gravityMultiplier);

    entity.add(rb);
}

void addComponentPlayerControl(const nlohmann::json& values, const nlohmann::json& allObjects,
                               const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData,
                               const ActiveLevel& level, ecs::Entity entity, LayerData layerData) {
    PlayerControl control = entity.has<PlayerControl>() ? entity.get<PlayerControl>() : DefaultPlayerControl;
    tryReadFloat(values, "speed", &control.moveSpeed);

    entity.add(control);
}

void addComponentJumper(const nlohmann::json& values, const nlohmann::json& allObjects,
                        const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, const ActiveLevel& level,
                        ecs::Entity entity, LayerData layerData) {
    Jumper jumper = entity.has<Jumper>() ? entity.get<Jumper>() : DefaultJumper;

    tryReadFloat(values, "jumpInitialVelocity", &jumper.jumpInitialVelocity);
    tryReadFloat(values, "jumpSecondsMax", &jumper.jumpSecondsMax);
    tryReadFloat(values, "coyoteTimeSecondsMax", &jumper.coyoteTimeSecondsMax);

    entity.add(jumper);
}

// TODO bad signature
Follow loadFollowComponent(const nlohmann::json& values, const ActiveLevel& level) {
    // Follow follow = entity.has<Follow>() ? entity.get<Follow>() : ComponentFactory::DefaultFollow;
    Follow follow = DefaultFollow;
    if (values.contains("FollowTarget")) {
        if (auto pPlayerSystem = System::world.getSystem<PlayerSystem>(); !pPlayerSystem->getEntitiesMutable().empty()) {
            follow.targetEntityID = pPlayerSystem->first().id();
        }
    }

    tryReadVector2f(values, "dampingX", "dampingY", &follow.damping);
    tryReadVector2i(values, "deadZoneX", "deadZoneY", &follow.deadZoneTexels);
    tryReadVector2i(values, "lookAheadX", "lookAheadY", &follow.lookAheadTexels);
    tryReadVector2i(values, "boundsHalflenX", "boundsHalflenX", &follow.boundsXTexels);
    tryReadVector2i(values, "boundsHalflenY", "boundsHalflenY", &follow.boundsYTexels);

    // convert bounds from local half length to world coords
    Vector2i levelPosTexels = Vector2i(level.worldOffsetPixels.x / PIXELS_PER_TEXEL, level.worldOffsetPixels.y / PIXELS_PER_TEXEL) +
                              Vector2i(level.sizeTexels.x / 2, level.sizeTexels.y / 2);
    follow.boundsXTexels = {levelPosTexels.x - follow.boundsXTexels.x, levelPosTexels.x + follow.boundsXTexels.x};
    follow.boundsYTexels = {levelPosTexels.y - follow.boundsYTexels.y, levelPosTexels.y + follow.boundsYTexels.y};
    return follow;
}

// returns true if checkpoints form a cycle
bool loadCheckpoints(const nlohmann::json& checkpointData, std::vector<RailsControl::CheckPoint>& dstCheckpoints, const ActiveLevel& level) {
    // generic rewrite:
    const s32 parentX = readInt(checkpointData, "x");
    const s32 parentY = readInt(checkpointData, "y");
    assert(checkpointData.contains("properties") && "Checkpoint object has no properties");
    const auto& properties = checkpointData["properties"];
    std::vector<Ease> moveProps;
    for (const auto& moveProperty : properties) {
        const s32 moveIx = moveProperty[KEY_VALUE];
        moveProps.push_back(static_cast<Ease>(moveIx));
    }

    bool isCycle = checkpointData.contains("polygon");
    std::string pathKey;
    if (isCycle) {
        pathKey = "polygon";
    } else {
        pathKey = "polyline";
    }
    size_t ix = 0;
    for (const auto& point : checkpointData[pathKey]) {
        const s32 x = readInt(point, "x");
        const s32 y = readInt(point, "y");
        const Vector2i mapPos = {x + parentX, parentY + y};
        const Vector2i trans = getTransformFromMapPosition(mapPos, {0, 0}, level, true).position;

        Ease moveType;
        if (ix >= moveProps.size()) {
            print("Checkpoints object with ID", readInt(checkpointData, "id"), "in level", level.filepath, "has", moveProps.size(),
                  "move type params but it has more points");
            moveType = Ease::Linear;
        } else {
            moveType = moveProps[ix];
        }
        RailsControl::CheckPoint chkPoint(trans, moveType);
        dstCheckpoints.push_back(chkPoint);

        ix++;
    }

    return isCycle;
}

void addTagComponents(const nlohmann::json& values, const nlohmann::json& allObjects,
                      const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, const ActiveLevel& level,
                      ecs::Entity entity, LayerData layerData) {
    bool hasTag = false;
    if (tryReadBool(values, "Player", &hasTag) && hasTag) {
        entity.add<Player>();
        hasTag = false;
    }

    if (tryReadBool(values, "PrecisePosition", &hasTag) && hasTag) {
        entity.add(PrecisePosition::fromTrans(entity.get<Transform2D>()));
        hasTag = false;
    }

    if (tryReadBool(values, "Wiggle", &hasTag) && hasTag) {
        entity.add<Wiggle>();
        hasTag = false;
    }

    if (tryReadBool(values, "Invisible", &hasTag) && hasTag) {
        entity.add<Invisible>();
        hasTag = false;
    }
}

void addSwitchComponent(const nlohmann::json& values, const nlohmann::json& allObjects,
                        const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, const ActiveLevel& level,
                        ecs::Entity entity, LayerData layerData) {
    s32 targetId;

    if (entityData.isParsingTemplate) {
        return;
    }

    if (!tryReadInt(values, "target", &targetId)) {
        print(entity.get<Name>(), "has Switch component with no target");
        assert(false);
    }

    MY_ASSERT(idToIndex.contains(targetId),
              whal_format("Target ID {} pointed to by {}'s SwitchComponent was not found", targetId, entity.get<Name>().name));
    ecs::Entity target = idToIndex.at(targetId).second;
    entity.add(Switch{target.id()});
}

void addSwitchGateComponent(const nlohmann::json& values, const nlohmann::json& allObjects,
                            const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, const ActiveLevel& level,
                            ecs::Entity entity, LayerData layerData) {
    SwitchGate gate = entity.has<SwitchGate>() ? entity.get<SwitchGate>() : DefaultSwitchGate;
    tryReadInt(values, "numKeys", &gate.numKeys);
    tryReadBool(values, "isPersistent", &gate.isPersistent);
    entity.add(gate);
}

void addComponentText(const nlohmann::json& values, const nlohmann::json& allObjects,
                      const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, const ActiveLevel& level,
                      ecs::Entity entity, LayerData layerData) {
    DrawText text = entity.has<DrawText>() ? entity.get<DrawText>() : DefaultDrawText;

    // ARGB
    if (values.contains("color")) {
        std::string hexcode = "#ffffffff";
        hexcode = values["color"];
        Color color = hexStringARGBToColor(hexcode);
        text.color = color;
    }

    tryReadString(values, "text", &text.text);
    tryReadBool(values, "center", &text.isCentered);
    text.frameSizeTexels = entityData.dimensionsTexels;
    entity.add(text);
}

void addComponentParticleEmitter(const nlohmann::json& values, const nlohmann::json& allObjects,
                                 const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData,
                                 const ActiveLevel& level, ecs::Entity entity, LayerData layerData) {
    ParticleEmitter emitter = entity.has<ParticleEmitter>() ? entity.get<ParticleEmitter>() : DefaultParticleEmitter;

    tryReadFloat(values, "maxSpeed", &emitter.maxSpeedTexelsPerSecond);
    tryReadInt(values, "particlesPerSecond", &emitter.particlesPerSecond);
    tryReadVal(values, "Direction", &emitter.direction);
    tryReadVal(values, "Material", &emitter.material);
    tryReadVal(values, "Depth", &emitter.depth);
    tryReadFloat(values, "LifetimeMultiplier", &emitter.lifetimeMultiplier);

    if (values.contains("Shape")) {
        s32 shapeId = readInt(values, "Shape");

        // calc distance between this object and Shape for the offset
        Vector2i halflenTexels = readVector2i(allObjects[idToIndex.at(shapeId).first], "width", "height") / 2;
        const Vector2i thisTrans = getTransformFromMapPosition(entityData.position, entityData.dimensionsTexels, level, entityData.isPoint).position;

        const auto& shapeObj = allObjects[idToIndex.at(shapeId).first];
        const Vector2i otherDimsTexels = readVector2i(shapeObj, "width", "height");
        const Vector2i otherTrans = getTransformFromMapPosition(readVector2i(shapeObj), otherDimsTexels, level, false).position;

        emitter.offsetTexels = otherTrans - thisTrans + Vector2i(0, halflenTexels.y);
        emitter.aabbHalfTexels = halflenTexels;
    } else {
        // emitter.offsetTexels = Vector2i(0, entityData.dimensionsTexels.y / 2);
        emitter.aabbHalfTexels = entityData.dimensionsTexels / 2;
    }

    entity.add(emitter);
}

void addComponentOrbit(const nlohmann::json& values, const nlohmann::json& allObjects,
                       const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, const ActiveLevel& level,
                       ecs::Entity entity, LayerData layerData) {
    Orbit orbit = entity.has<Orbit>() ? entity.get<Orbit>() : DefaultOrbit;

    tryReadFloat(values, "RotationsPerSecond", &orbit.rotationsPerSecond);

    const Vector2i entityDimensions = entityData.dimensionsTexels;
    const Vector2i entityTrans = entity.get<Transform2D>().position;

    if (!values.contains("Target")) {
        print("Error: Orbit component requires a Target");
        return;
    }

    s32 shapeId = readInt(values, "Target");
    const auto& shapeObj = allObjects[idToIndex.at(shapeId).first];
    Vector2i otherDimensions = Vector2i::zero;
    bool isPoint = true;
    if (tryReadVector2i(shapeObj, "width", "height", &otherDimensions)) {
        isPoint = false;
    }
    const Vector2i otherTrans = getTransformFromMapPosition(readVector2i(shapeObj), otherDimensions, level, isPoint).position;

    orbit.radius = std::round((entityTrans - otherTrans).as<f32>().len());

    // seems to work correctly without this, actually
    // orbit.targetOffset = entityDimensions / 2 - otherDimensions / 2;
    // orbit.selfOffset = Vector2i(0, entityDimensions.y / 2);  // use the entity's center for orbiting
    orbit.targetID = idToIndex.at(shapeId).second.id();

    entity.add(orbit);
}

s32 readInt(const nlohmann::json& data, std::string_view key) {
    MY_ASSERT(data.contains(key), whal_format("Missing key: {}", key).c_str());
    return data[key];
}

s32 readFloat(const nlohmann::json& data, std::string_view key) {
    MY_ASSERT(data.contains(key), whal_format("Missing key: {}", key).c_str());
    return data[key];
}

Vector2i readVector2i(const nlohmann::json& data, const char* xKey, const char* yKey) {
    MY_ASSERT(data.contains(xKey), data.contains(yKey) && whal_format("Missing keys: {} & {}", xKey, yKey).c_str());
    return Vector2i(data[xKey], data[yKey]);
}

bool readBool(const nlohmann::json& data, std::string_view key) {
    MY_ASSERT(data.contains(key), whal_format("Missing key: {}", key).c_str());
    return data[key];
}

std::string readString(const nlohmann::json& data, std::string_view key) {
    MY_ASSERT(data.contains(key), whal_format("Missing key: {}", key).c_str());
    return data[key];
}

template <typename T>
T readVal(const nlohmann::json& data, std::string_view key) {
    MY_ASSERT(data.contains(key), whal_format("Missing key: {}", key).c_str());
    return data[key];
}

bool tryReadInt(const nlohmann::json& data, std::string_view key, s32* dst) {
    if (data.contains(key)) {
        *dst = data[key];
        return true;
    }
    return false;
}

bool tryReadFloat(const nlohmann::json& data, std::string_view key, f32* dst) {
    if (data.contains(key)) {
        *dst = data[key];
        return true;
    }
    return false;
}

bool tryReadVector2i(const nlohmann::json& data, std::string_view xKey, std::string_view yKey, Vector2i* dst) {
    bool foundOne = false;
    if (data.contains(xKey)) {
        dst->x = data[xKey];
        foundOne = true;
    }
    if (data.contains(yKey)) {
        dst->y = data[yKey];
        foundOne = true;
    }
    return foundOne;
}

bool tryReadVector2f(const nlohmann::json& data, std::string_view xKey, std::string_view yKey, Vector2f* dst) {
    bool foundOne = false;
    if (data.contains(xKey)) {
        dst->x = data[xKey];
        foundOne = true;
    }
    if (data.contains(yKey)) {
        dst->y = data[yKey];
        foundOne = true;
    }
    return foundOne;
}

bool tryReadBool(const nlohmann::json& data, std::string_view key, bool* dst) {
    if (data.contains(key)) {
        *dst = data[key];
        return true;
    }
    return false;
}

bool tryReadString(const nlohmann::json& data, std::string_view key, std::string* dst) {
    if (data.contains(key)) {
        *dst = data[key];
        return true;
    }
    return false;
}

template <typename T>
bool tryReadVal(const nlohmann::json& data, std::string_view key, T* dst) {
    if (data.contains(key)) {
        *dst = data[key];
        return true;
    }
    return false;
}

}  // namespace whal

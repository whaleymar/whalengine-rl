#include "ComponentFactory.h"

#include "CorradeOptional.h"
#include "Map/AnimationFactory.h"
#include "json.hpp"

#include "Util/Vector.h"
#include "whalECS/src/ECS.h"

#include "Components/Animator.h"
#include "Components/Collision.h"
#include "Components/Draw.h"
#include "Components/GfxFlags.h"
#include "Components/Lifetime.h"
#include "Components/Light.h"
#include "Components/ParticleEmitter.h"
#include "Components/PlayerControl.h"
#include "Components/RailsControl.h"
#include "Components/RigidBody.h"
#include "Components/Tags.h"
#include "Components/Transform.h"
#include "Components/TriggerZone.h"
#include "Components/Velocity.h"

#include "Map/Level.h"
#include "Map/Tiled.h"

#include "Physics/CollisionLayer.h"

#include "Util/DebugUtil.h"
#include "Util/Print.h"

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
static void addComponentLight(const nlohmann::json& values, const nlohmann::json& allObjects,
                              const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData,
                              const ActiveLevel& level, ecs::Entity entity, LayerData layerData);
static void addComponentRadiance(const nlohmann::json& values, const nlohmann::json& allObjects,
                                 const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData,
                                 const ActiveLevel& level, ecs::Entity entity, LayerData layerData);
static void addComponentLifetime(const nlohmann::json& values, const nlohmann::json& allObjects,
                                 const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData,
                                 const ActiveLevel& level, ecs::Entity entity, LayerData layerData);
static void addComponentAttach(const nlohmann::json& values, const nlohmann::json& allObjects,
                               const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData,
                               const ActiveLevel& level, ecs::Entity entity, LayerData layerData);
static void addComponentOrbit(const nlohmann::json& values, const nlohmann::json& allObjects,
                              const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData,
                              const ActiveLevel& level, ecs::Entity entity, LayerData layerData);

static bool loadCheckpoints(const nlohmann::json& checkpointData, std::vector<RailsControl::CheckPoint>& dstCheckpoints, const ActiveLevel& level);

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
static Attach DefaultAttach;
static PointLight DefaultPointLight;
static Radiance DefaultRadiance;
static Lifetime DefaultLifeTime;
static DrawText DefaultDrawText;
static ParticleEmitter DefaultParticleEmitter;
static Orbit DefaultOrbit;

static const NameToCreator<ComponentAdder> S_COMPONENT_ENTRIES[] = {
    {"Component_RailsControl", addComponentRailsControl},
    {"Component_Collider", addComponentCollider},
    {"Component_Trigger", addComponentTrigger},
    {"Component_Draw", addComponentDraw},
    {"Component_Sprite_NoAnim", addComponentSprite},
    {"Component_Sprite_Animated", addComponentAnimator},
    {"Component_DrawLayer_TileOnly", addDrawLayer},
    {"Component_PointLight", addComponentLight},
    {"Component_Radiance", addComponentRadiance},
    {"Component_Lifetime", addComponentLifetime},
    {"Component_Attach", addComponentAttach},
    {"Component_RigidBody", addComponentRigidBody},
    {"Component_PlayerControl", addComponentPlayerControl},
    {"Component_Jumper", addComponentJumper},
    {"Component_Velocity", addComponentVelocity},
    {"Component_Tags", addTagComponents},
    {"Component_Text", addComponentText},
    {"Component_ParticleEmitter", addComponentParticleEmitter},
    {"Component_Orbit", addComponentOrbit},
};

ComponentFactory::ComponentFactory() : DynamicFactory<ComponentAdder>("ComponentFactory", S_COMPONENT_ENTRIES) {}

void ComponentFactory::makeDefaultComponent(const nlohmann::json& property) {
    std::string componentName = property[KEY_NAME];
    ComponentAdder creatorFunc = nullptr;
    if (!getEntry(componentName.c_str(), &creatorFunc)) {
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
                DefaultDraw.color = parseColor(hexString);
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
                DefaultSprite.color = parseColor(hexString);
            } else if (memberName == "Sprite") {
                // do nothing
            } else if (memberName == "rotationDegrees") {
                // do nothing, affects Transform
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
                DefaultAnimatedSprite.color = parseColor(hexString);
            } else if (memberName == "Sprite") {
                // do nothing
            } else if (memberName == "rotationDegrees") {
                // do nothing, affects Transform
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
                DefaultPointLight.color = parseColor(hexString);
            } else if (memberName == "heightTexels") {
                DefaultPointLight.heightOffset = member[KEY_VALUE];
            } else if (memberName == "radiusTexels") {
                DefaultPointLight.radius = member[KEY_VALUE];
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
                DefaultRadiance.color = parseColor(hexString);
            } else if (memberName == "heightTexels") {
                DefaultRadiance.heightOffset = member[KEY_VALUE];
            } else if (memberName == "radiusTexels") {
                DefaultRadiance.radius = member[KEY_VALUE];
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

    } else if (componentName == "Component_Text") {
        DefaultDrawText = DrawText{};
        for (const auto& member : property[KEY_MEMBERS]) {
            std::string memberName = member[KEY_NAME];
            if (memberName == "text") {
                DefaultDrawText.text = member[KEY_VALUE];
            } else if (memberName == "color") {
                std::string hexString = member[KEY_VALUE];
                DefaultDrawText.color = parseColor(hexString);
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
                DefaultParticleEmitter.maxSpeed = member[KEY_VALUE];

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
            flags |= GfxFlags::Bloom;
        } else if (textureLayer == "Glow") {
            flags |= GfxFlags::Glow;
        }
    }
    return flags;
}

void addComponentDraw(const nlohmann::json& values, const nlohmann::json& allObjects,
                      const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, const ActiveLevel& level,
                      ecs::Entity entity, LayerData layerData) {
    DrawRect draw = entity.has<DrawRect>() ? entity.get<DrawRect>() : DefaultDraw;
    draw.frameSize = entityData.size;

    // ARGB
    if (values.contains("Color")) {
        std::string hexcode = "#ffffffff";
        hexcode = values["Color"];
        Color color = parseColor(hexcode);
        draw.color = color;
    }

    u32 flags = parseGfxEffects(values);
    entity.add(GfxFlags{flags});
    entity.add(draw);
}

void addComponentSprite(const nlohmann::json& values, const nlohmann::json& allObjects,
                        const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, const ActiveLevel& level,
                        ecs::Entity entity, LayerData layerData) {
    Sprite sprite = entity.has<Sprite>() ? entity.get<Sprite>() : DefaultSprite;

    s32 rotationDegrees;
    if (tryReadInt(values, "rotationDegrees", &rotationDegrees)) {
        entity.get<Transform2D>().rotationDegrees = rotationDegrees;
    }

    // ARGB
    if (values.contains("Color")) {
        std::string hexcode = "#ffffffff";
        hexcode = values["Color"];
        Color color = parseColor(hexcode);
        sprite.color = color;
    }

    u32 flags = parseGfxEffects(values);
    entity.add(GfxFlags{flags});

    std::string spritePath = "";
    if (values.contains("Sprite")) {
        spritePath = values["Sprite"];
        std::replace(spritePath.begin(), spritePath.end(), '\\', '/');
    }
    auto eSprite = Sprite::fromPath(spritePath.c_str());
    if (eSprite.isExpected()) {
        sprite.frameSize = eSprite.value().frameSize;
        sprite.atlasPosition = eSprite.value().atlasPosition;
        entity.add(sprite);
    } else {
        print("Error: Coudn't find frame for sprite:", spritePath);
    }
}

void addComponentAnimator(const nlohmann::json& values, const nlohmann::json& allObjects,
                          const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, const ActiveLevel& level,
                          ecs::Entity entity, LayerData layerData) {
    Sprite sprite = entity.has<Sprite>() ? entity.get<Sprite>() : DefaultAnimatedSprite;
    std::string animatorName = readString(values, "Animator");
    Animator animator = AnimationFactory::get(animatorName.c_str());
    entity.add(animator);
    sprite.setFrame(animator.getFrame());

    s32 rotationDegrees;
    if (tryReadInt(values, "rotationDegrees", &rotationDegrees)) {
        entity.get<Transform2D>().rotationDegrees = rotationDegrees;
    }

    // ARGB
    if (values.contains("Color")) {
        std::string hexcode = "#ffffffff";
        hexcode = values["Color"];
        Color color = parseColor(hexcode);
        sprite.color = color;
    }

    u32 flags = parseGfxEffects(values);
    entity.add(GfxFlags{flags});

    entity.add(sprite);
}

// this is only intended to be used on tiles (which already have a sprite), not objects
void addDrawLayer(const nlohmann::json& values, const nlohmann::json& allObjects,
                  const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, const ActiveLevel& level,
                  ecs::Entity entity, LayerData layerData) {
    if (!entity.has<Sprite>()) {
        print("can't add draw layer for entity", entityData.id, "without Draw component");
        return;
    }

    u32 flags = parseGfxEffects(values);
    entity.add(GfxFlags{flags});
}

void addComponentLight(const nlohmann::json& values, const nlohmann::json& allObjects,
                       const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, const ActiveLevel& level,
                       ecs::Entity entity, LayerData layerData) {
    PointLight light = entity.has<PointLight>() ? entity.get<PointLight>() : DefaultPointLight;
    if (!tryReadVal(values, "radiusTexels", &light.radius)) {
        // by default, use bigger dimension
        light.radius = std::max(entityData.size.x, entityData.size.y);
    }
    if (!tryReadVal(values, "heightTexels", &light.heightOffset)) {
        // by default, use half of entity height
        light.heightOffset = entityData.size.y / 2;
    }
    std::string hexString;
    if (tryReadVal(values, "Color", &hexString)) {
        light.color = parseColor(hexString);
    }
    entity.add(light);
}

void addComponentRadiance(const nlohmann::json& values, const nlohmann::json& allObjects,
                          const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, const ActiveLevel& level,
                          ecs::Entity entity, LayerData layerData) {
    Radiance light = entity.has<Radiance>() ? entity.get<Radiance>() : DefaultRadiance;
    if (!tryReadVal(values, "radiusTexels", &light.radius)) {
        // by default, use bigger dimension
        light.radius = std::max(entityData.size.x, entityData.size.y);
    }
    if (!tryReadVal(values, "heightTexels", &light.heightOffset)) {
        // by default, use half of entity height
        light.heightOffset = entityData.size.y / 2;
    }
    std::string hexString;
    if (tryReadVal(values, "Color", &hexString)) {
        light.color = parseColor(hexString);
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
        const Vector2i otherDims = readVector2i(shapeObj, "width", "height");
        const Vector2i halflen = otherDims / 2;
        const Vector2i thisTrans = getTransformFromMapPosition(entityData.position, entityData.size, level, entityData.isPoint).position;

        const Vector2i otherTrans = getTransformFromMapPosition(readVector2i(shapeObj), otherDims, level, false).position;

        const auto offset = otherTrans - thisTrans;
        Transform2D transOffset = entity.get<Transform2D>();
        if (!offset.isZero()) {
            entity.add(ColliderOffset(offset));
            transOffset.position += offset;
        }
        collider.setShape(AABB(transOffset, halflen));
    } else {
        // there's no default shape object. Instead use the entity's dimensions
        collider.setShape(AABB(entity.get<Transform2D>(), entityData.size / 2));
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
        const Vector2i otherDims = readVector2i(shapeObj, "width", "height");
        const Vector2i halflen = otherDims / 2;
        const Vector2i thisTrans = getTransformFromMapPosition(entityData.position, entityData.size, level, entityData.isPoint).position;

        const Vector2i otherTrans = getTransformFromMapPosition(readVector2i(shapeObj), otherDims, level, false).position;

        trigger.offset = otherTrans - thisTrans;

        if (shapeObj.contains("ellipse")) {
            const s32 radius = std::max(halflen.x, halflen.y);
            trigger.shape = Circle(entity.get<Transform2D>().position + trigger.offset + Vector2i(0, halflen.y), radius);

        } else {
            trigger.shape = AABB(entity.get<Transform2D>().position + trigger.offset + Vector2i(0, halflen.y), halflen);
        }
    } else {
        if (allObjects[idToIndex.at(entityData.id).first].contains("ellipse")) {
            const s32 radius = std::max(entityData.size.x, entityData.size.y) / 2;
            trigger.shape = Circle(entity.get<Transform2D>(), radius);

        } else {
            trigger.shape = AABB(entity.get<Transform2D>(), entityData.size / 2);
        }
    }

    entity.add(trigger);
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
    Vector2i otherDims = getObjectSize(targetObj);
    const Vector2i otherPosition = getTransformFromMapPosition(readVector2i(targetObj), otherDims, level, false).position;

    attach.offset = (thisPosition - otherPosition);
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

    if (tryReadBool(values, "BlocksLight", &hasTag) && hasTag) {
        entity.add<BlocksLight>();
        hasTag = false;
    }
}

void addComponentText(const nlohmann::json& values, const nlohmann::json& allObjects,
                      const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, const ActiveLevel& level,
                      ecs::Entity entity, LayerData layerData) {
    DrawText text = entity.has<DrawText>() ? entity.get<DrawText>() : DefaultDrawText;

    // ARGB
    if (values.contains("color")) {
        std::string hexcode = "#ffffffff";
        hexcode = values["color"];
        Color color = parseColor(hexcode);
        text.color = color;
    }

    tryReadString(values, "text", &text.text);
    tryReadBool(values, "center", &text.isCentered);
    text.frameSize = entityData.size;
    entity.add(text);
}

void addComponentParticleEmitter(const nlohmann::json& values, const nlohmann::json& allObjects,
                                 const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData,
                                 const ActiveLevel& level, ecs::Entity entity, LayerData layerData) {
    ParticleEmitter emitter = entity.has<ParticleEmitter>() ? entity.get<ParticleEmitter>() : DefaultParticleEmitter;

    tryReadFloat(values, "maxSpeed", &emitter.maxSpeed);
    tryReadInt(values, "particlesPerSecond", &emitter.particlesPerSecond);
    tryReadVal(values, "Direction", &emitter.direction);
    tryReadVal(values, "Material", &emitter.material);
    tryReadVal(values, "Depth", &emitter.depth);
    tryReadFloat(values, "LifetimeMultiplier", &emitter.lifetimeMultiplier);

    if (values.contains("Shape")) {
        s32 shapeId = readInt(values, "Shape");

        // calc distance between this object and Shape for the offset
        Vector2i halflen = readVector2i(allObjects[idToIndex.at(shapeId).first], "width", "height") / 2;
        const Vector2i thisTrans = getTransformFromMapPosition(entityData.position, entityData.size, level, entityData.isPoint).position;

        const auto& shapeObj = allObjects[idToIndex.at(shapeId).first];
        const Vector2i otherDims = readVector2i(shapeObj, "width", "height");
        const Vector2i otherTrans = getTransformFromMapPosition(readVector2i(shapeObj), otherDims, level, false).position;

        emitter.offset = otherTrans - thisTrans + Vector2i(0, halflen.y);
        emitter.aabbHalf = halflen;
    } else {
        // emitter.offset = Vector2i(0, entityData.dimensions.y / 2);
        emitter.aabbHalf = entityData.size / 2;
    }

    entity.add(emitter);
}

void addComponentOrbit(const nlohmann::json& values, const nlohmann::json& allObjects,
                       const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, const ActiveLevel& level,
                       ecs::Entity entity, LayerData layerData) {
    Orbit orbit = entity.has<Orbit>() ? entity.get<Orbit>() : DefaultOrbit;

    tryReadFloat(values, "RotationsPerSecond", &orbit.rotationsPerSecond);

    const Vector2i entityDimensions = entityData.size;
    const Vector2i entityTrans = entity.get<Transform2D>().position;

    if (!values.contains("Target")) {
        print("Error: Orbit component requires a Target");
        return;
    }

    s32 shapeId = readInt(values, "Target");
    const auto& shapeObj = allObjects[idToIndex.at(shapeId).first];
    Vector2i otherDimensions = Vector2i::ZERO;
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
    DBG_ASSERT(data.contains(key), whal_format("Missing key: {}", key).c_str());
    return data[key];
}

s32 readFloat(const nlohmann::json& data, std::string_view key) {
    DBG_ASSERT(data.contains(key), whal_format("Missing key: {}", key).c_str());
    return data[key];
}

Vector2i readVector2i(const nlohmann::json& data, const char* xKey, const char* yKey) {
    DBG_ASSERT(data.contains(xKey), data.contains(yKey) && whal_format("Missing keys: {} & {}", xKey, yKey).c_str());
    return Vector2i(data[xKey], data[yKey]);
}

bool readBool(const nlohmann::json& data, std::string_view key) {
    DBG_ASSERT(data.contains(key), whal_format("Missing key: {}", key).c_str());
    return data[key];
}

std::string readString(const nlohmann::json& data, std::string_view key) {
    DBG_ASSERT(data.contains(key), whal_format("Missing key: {}", key).c_str());
    return data[key];
}

template <typename T>
T readVal(const nlohmann::json& data, std::string_view key) {
    DBG_ASSERT(data.contains(key), whal_format("Missing key: {}", key).c_str());
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

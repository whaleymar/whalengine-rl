#include "ComponentFactory.h"

#include "CorradeOptional.h"
#include "Map/AnimationFactory.h"
#include "json.hpp"

#include "Util/Vector.h"
#include "whalECS/src/ECS.h"

#include "Components/Animator.h"
#include "Components/Collision.h"
#include "Components/Draw.h"
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

static void addTagComponents(ecs::Entity entity, const LoadContext& ctx);
static void addComponentVelocity(ecs::Entity entity, const LoadContext& ctx);
static void addComponentRailsControl(ecs::Entity entity, const LoadContext& ctx);
static void addComponentCollider(ecs::Entity entity, const LoadContext& ctx);
static void addComponentTrigger(ecs::Entity entity, const LoadContext& ctx);
static void addComponentRigidBody(ecs::Entity entity, const LoadContext& ctx);
static void addComponentPlayerControl(ecs::Entity entity, const LoadContext& ctx);
static void addComponentJumper(ecs::Entity entity, const LoadContext& ctx);
static void addComponentDraw(ecs::Entity entity, const LoadContext& ctx);
static void addComponentSprite(ecs::Entity entity, const LoadContext& ctx);
static void addComponentAnimator(ecs::Entity entity, const LoadContext& ctx);
static void addComponentLight(ecs::Entity entity, const LoadContext& ctx);
static void addComponentLifetime(ecs::Entity entity, const LoadContext& ctx);
static void addComponentAttach(ecs::Entity entity, const LoadContext& ctx);
static void addComponentOrbit(ecs::Entity entity, const LoadContext& ctx);
static bool loadCheckpoints(const nlohmann::json& checkpointData, std::vector<RailsControl::CheckPoint>& dstCheckpoints, const ActiveLevel& level);
static void addComponentText(ecs::Entity entity, const LoadContext& ctx);
static void addComponentParticleEmitter(ecs::Entity entity, const LoadContext& ctx);

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
static Lifetime DefaultLifeTime;
static DrawText DefaultDrawText;
static ParticleEmitter DefaultParticleEmitter;
static Orbit DefaultOrbit;

static const NameToCreator<ComponentAdder> S_COMPONENT_ENTRIES[] = {
    {"Component_RailsControl", addComponentRailsControl},
    {"Component_Collider", addComponentCollider},
    {"Component_Trigger", addComponentTrigger},
    {"Component_DrawRect", addComponentDraw},
    {"Component_Sprite_NoAnim", addComponentSprite},
    {"Component_Sprite_Animated", addComponentAnimator},
    {"Component_PointLight", addComponentLight},
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
                DefaultParticleEmitter.depth = parseDepth(member[KEY_VALUE]);

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

void addComponentVelocity(ecs::Entity entity, const LoadContext& ctx) {
    Velocity velocity = entity.has<Velocity>() ? entity.get<Velocity>() : DefaultVelocity;
    tryReadVector2f(ctx.values, "velX", "velY", &velocity.stable);

    entity.add(velocity);
}

void addComponentRailsControl(ecs::Entity entity, const LoadContext& ctx) {
    std::vector<RailsControl::CheckPoint> checkpoints;
    bool isCycle = false;
    if (ctx.values.contains("Checkpoints")) {
        s32 id = ctx.values["Checkpoints"];
        const nlohmann::json checkPointObj = ctx.allObjects.at(ctx.idToIndex.at(id).first);
        isCycle = loadCheckpoints(checkPointObj, checkpoints, ctx.level);
    }

    RailsControl rails = entity.has<RailsControl>() ? entity.get<RailsControl>() : DefaultRailsControl;
    rails.setCheckpoints(checkpoints, entity.get<Transform>());

    std::string cycleBehavior = "ManualStart";
    tryReadString(ctx.values, "CycleBehavior", &cycleBehavior);
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

    tryReadFloat(ctx.values, "speed", &rails.speed);
    tryReadFloat(ctx.values, "waitTime", &rails.waitTime);

    entity.add(rails);
}

void addComponentDraw(ecs::Entity entity, const LoadContext& ctx) {
    DrawRect draw = entity.has<DrawRect>() ? entity.get<DrawRect>() : DefaultDraw;
    draw.frameSize = ctx.entityData.size;

    // ARGB
    if (ctx.values.contains("Color")) {
        std::string hexcode = "#ffffffff";
        hexcode = ctx.values["Color"];
        Color color = parseColor(hexcode);
        draw.color = color;
    }

    f32 brightness;
    if (tryReadFloat(ctx.values, "Brightness", &brightness)) {
        draw.color.scale(brightness);
    }
    entity.add(draw);
}

void addComponentSprite(ecs::Entity entity, const LoadContext& ctx) {
    Sprite sprite = entity.has<Sprite>() ? entity.get<Sprite>() : DefaultSprite;

    s32 rotationDegrees;
    if (tryReadInt(ctx.values, "rotationDegrees", &rotationDegrees)) {
        entity.get<Transform>().rotationDegrees = rotationDegrees;
    }

    // ARGB
    if (ctx.values.contains("Color")) {
        std::string hexcode = "#ffffffff";
        hexcode = ctx.values["Color"];
        Color color = parseColor(hexcode);
        sprite.color = color;
    }

    f32 brightness;
    if (tryReadFloat(ctx.values, "Brightness", &brightness)) {
        sprite.color.scale(brightness);
    }

    std::string spritePath = "";
    if (ctx.values.contains("Sprite")) {
        spritePath = ctx.values["Sprite"];
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

void addComponentAnimator(ecs::Entity entity, const LoadContext& ctx) {
    Sprite sprite = entity.has<Sprite>() ? entity.get<Sprite>() : DefaultAnimatedSprite;
    std::string animatorName = readString(ctx.values, "Animator");
    Animator animator = AnimationFactory::get(animatorName.c_str());
    entity.add(animator);
    sprite.setFrame(animator.getFrame());

    s32 rotationDegrees;
    if (tryReadInt(ctx.values, "rotationDegrees", &rotationDegrees)) {
        entity.get<Transform>().rotationDegrees = rotationDegrees;
    }

    // ARGB
    if (ctx.values.contains("Color")) {
        std::string hexcode = "#ffffffff";
        hexcode = ctx.values["Color"];
        Color color = parseColor(hexcode);
        sprite.color = color;
    }

    f32 brightness;
    if (tryReadFloat(ctx.values, "Brightness", &brightness)) {
        sprite.color.scale(brightness);
    }
    entity.add(sprite);
}

void addComponentLight(ecs::Entity entity, const LoadContext& ctx) {
    PointLight light = entity.has<PointLight>() ? entity.get<PointLight>() : DefaultPointLight;
    if (!tryReadVal(ctx.values, "radiusTexels", &light.radius)) {
        // by default, use bigger dimension
        light.radius = std::max(ctx.entityData.size.x, ctx.entityData.size.y);
    }
    tryReadVal(ctx.values, "heightTexels", &light.heightOffset);
    std::string hexString;
    if (tryReadVal(ctx.values, "Color", &hexString)) {
        light.color = parseColor(hexString);
    }
    entity.add(light);
}

void addComponentLifetime(ecs::Entity entity, const LoadContext& ctx) {
    Lifetime lifetime = entity.has<Lifetime>() ? entity.get<Lifetime>() : DefaultLifeTime;
    tryReadVal(ctx.values, "seconds", &lifetime.secondsRemaining);
    entity.add(lifetime);
}

void addComponentCollider(ecs::Entity entity, const LoadContext& ctx) {
    Collider collider = entity.has<Collider>() ? entity.get<Collider>() : DefaultCollider;
    CollisionDir collisionDir = collider.getCollisionDir();
    WorldMaterial material = collider.getMaterial();

    if (tryReadVal(ctx.values, "CollisionDir", &collisionDir)) {
        collider.setCollisionDir(collisionDir);
    }
    if (tryReadVal(ctx.values, "Material", &material)) {
        collider.setMaterial(material);
    }

    std::string layerName;
    if (tryReadVal(ctx.values, "Layer", &layerName)) {
        collider.setCollisionLayer(CollisionLayer::fromString(layerName.c_str()));
    }

    if (ctx.values.contains("Shape")) {
        s32 shapeId = readInt(ctx.values, "Shape");
        // calc distance between this object and Shape for the offset
        const auto& shapeObj = ctx.allObjects[ctx.idToIndex.at(shapeId).first];
        const Vector2i otherDims = readVector2i(shapeObj, "width", "height");
        const Vector2i halflen = otherDims / 2;
        const Vector2i thisTrans =
            getTransformFromMapPosition(ctx.entityData.position, ctx.entityData.size, ctx.level, ctx.entityData.isPoint).position;

        const Vector2i otherTrans = getTransformFromMapPosition(readVector2i(shapeObj), otherDims, ctx.level, false).position;

        const auto offset = otherTrans - thisTrans;
        Transform transOffset = entity.get<Transform>();
        collider.setShape(AABB(transOffset, halflen, offset));
    } else {
        // there's no default shape object. Instead use the entity's dimensions
        collider.setShape(AABB(entity.get<Transform>(), ctx.entityData.size / 2, Vector2i()));
    }
    entity.add(collider);
}

void addComponentTrigger(ecs::Entity entity, const LoadContext& ctx) {
    Trigger trigger = entity.has<Trigger>() ? entity.get<Trigger>() : DefaultTrigger;

    std::string layerName;
    if (tryReadVal(ctx.values, "Layer", &layerName)) {
        trigger.layer = CollisionLayer::fromString(layerName.c_str());
    }

    if (ctx.values.contains("Shape")) {
        s32 shapeId = readInt(ctx.values, "Shape");
        // calc distance between this object and Shape for the offset
        const auto& shapeObj = ctx.allObjects[ctx.idToIndex.at(shapeId).first];
        const Vector2i otherDims = readVector2i(shapeObj, "width", "height");
        const Vector2i halflen = otherDims / 2;
        const Vector2i thisTrans =
            getTransformFromMapPosition(ctx.entityData.position, ctx.entityData.size, ctx.level, ctx.entityData.isPoint).position;

        const Vector2i otherTrans = getTransformFromMapPosition(readVector2i(shapeObj), otherDims, ctx.level, false).position;

        trigger.offset = otherTrans - thisTrans;

        if (shapeObj.contains("ellipse")) {
            const s32 radius = std::max(halflen.x, halflen.y);
            trigger.shape = Circle(entity.get<Transform>().position + trigger.offset + Vector2i(0, halflen.y), radius);

        } else {
            trigger.shape = AABB(entity.get<Transform>().position + trigger.offset + Vector2i(0, halflen.y), halflen);
        }
    } else {
        // TODO offsets, like i do w/ colliders
        if (ctx.allObjects[ctx.idToIndex.at(ctx.entityData.id).first].contains("ellipse")) {
            const s32 radius = std::max(ctx.entityData.size.x, ctx.entityData.size.y) / 2;
            trigger.shape = Circle(entity.get<Transform>(), radius);

        } else {
            trigger.shape = AABB(entity.get<Transform>(), ctx.entityData.size / 2, Vector2i());
        }
    }

    entity.add(trigger);
}

void addComponentAttach(ecs::Entity entity, const LoadContext& ctx) {
    Attach attach = entity.has<Attach>() ? entity.get<Attach>() : DefaultAttach;

    s32 targetId;
    if (!tryReadInt(ctx.values, "target", &targetId)) {
        print("Entity with Map id ", ctx.entityData.id, "has attach component with no target");
        return;
    }

    tryReadVal(ctx.values, "DirectionParam", &attach.directionParam);

    ecs::Entity target = ctx.idToIndex.at(targetId).second;
    attach.targetEntityID = target.id();

    auto thisPosition = entity.get<Transform>().position;

    // other isn't guaranteed to have been parsed. Calculate its transform manually
    const auto& targetObj = ctx.allObjects[ctx.idToIndex.at(targetId).first];
    Vector2i otherDims = getObjectSize(targetObj);
    const Vector2i otherPosition = getTransformFromMapPosition(readVector2i(targetObj), otherDims, ctx.level, false).position;

    attach.offset = (thisPosition - otherPosition);
    entity.add(attach);
}

void addComponentRigidBody(ecs::Entity entity, const LoadContext& ctx) {
    RigidBody rb = entity.has<RigidBody>() ? entity.get<RigidBody>() : DefaultRigidBody;

    tryReadVector2f(ctx.values, "momentumMultiplierX", "momentumMultiplierY", &rb.momentumMultiplier);
    tryReadVector2f(ctx.values, "frictionGround", "frictionAir", &rb.frictionMultiplier);
    tryReadFloat(ctx.values, "gravityMultiplier", &rb.gravityMultiplier);

    entity.add(rb);
}

void addComponentPlayerControl(ecs::Entity entity, const LoadContext& ctx) {
    PlayerControl control = entity.has<PlayerControl>() ? entity.get<PlayerControl>() : DefaultPlayerControl;
    tryReadFloat(ctx.values, "speed", &control.moveSpeed);

    entity.add(control);
}

void addComponentJumper(ecs::Entity entity, const LoadContext& ctx) {
    Jumper jumper = entity.has<Jumper>() ? entity.get<Jumper>() : DefaultJumper;

    tryReadFloat(ctx.values, "jumpInitialVelocity", &jumper.jumpInitialVelocity);
    tryReadFloat(ctx.values, "jumpSecondsMax", &jumper.jumpSecondsMax);
    tryReadFloat(ctx.values, "coyoteTimeSecondsMax", &jumper.coyoteTimeSecondsMax);

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

void addTagComponents(ecs::Entity entity, const LoadContext& ctx) {
    bool hasTag = false;
    if (tryReadBool(ctx.values, "Player", &hasTag) && hasTag) {
        entity.add<Player>();
        hasTag = false;
    }

    if (tryReadBool(ctx.values, "PrecisePosition", &hasTag) && hasTag) {
        entity.add(PrecisePosition::fromTrans(entity.get<Transform>()));
        hasTag = false;
    }

    if (tryReadBool(ctx.values, "Wiggle", &hasTag) && hasTag) {
        entity.add<Wiggle>();
        hasTag = false;
    }

    if (tryReadBool(ctx.values, "Invisible", &hasTag) && hasTag) {
        entity.add<Invisible>();
        hasTag = false;
    }

    if (tryReadBool(ctx.values, "BlocksLight", &hasTag) && hasTag) {
        entity.add<BlocksLight>();
        hasTag = false;
    }
}

void addComponentText(ecs::Entity entity, const LoadContext& ctx) {
    DrawText text = entity.has<DrawText>() ? entity.get<DrawText>() : DefaultDrawText;

    // ARGB
    if (ctx.values.contains("color")) {
        std::string hexcode = "#ffffffff";
        hexcode = ctx.values["color"];
        Color color = parseColor(hexcode);
        text.color = color;
    }

    tryReadString(ctx.values, "text", &text.text);
    tryReadBool(ctx.values, "center", &text.isCentered);
    text.frameSize = ctx.entityData.size;
    entity.add(text);
}

void addComponentParticleEmitter(ecs::Entity entity, const LoadContext& ctx) {
    ParticleEmitter emitter = entity.has<ParticleEmitter>() ? entity.get<ParticleEmitter>() : DefaultParticleEmitter;

    tryReadFloat(ctx.values, "maxSpeed", &emitter.maxSpeed);
    tryReadInt(ctx.values, "particlesPerSecond", &emitter.particlesPerSecond);
    tryReadVal(ctx.values, "Direction", &emitter.direction);
    tryReadVal(ctx.values, "Material", &emitter.material);
    tryReadDepth(ctx.values, "Depth", &emitter.depth);
    tryReadFloat(ctx.values, "LifetimeMultiplier", &emitter.lifetimeMultiplier);

    if (ctx.values.contains("Shape")) {
        s32 shapeId = readInt(ctx.values, "Shape");

        // calc distance between this object and Shape for the offset
        Vector2i halflen = readVector2i(ctx.allObjects[ctx.idToIndex.at(shapeId).first], "width", "height") / 2;
        const Vector2i thisTrans =
            getTransformFromMapPosition(ctx.entityData.position, ctx.entityData.size, ctx.level, ctx.entityData.isPoint).position;

        const auto& shapeObj = ctx.allObjects[ctx.idToIndex.at(shapeId).first];
        const Vector2i otherDims = readVector2i(shapeObj, "width", "height");
        const Vector2i otherTrans = getTransformFromMapPosition(readVector2i(shapeObj), otherDims, ctx.level, false).position;

        emitter.offset = otherTrans - thisTrans + Vector2i(0, halflen.y);
        emitter.aabbHalf = halflen;
    } else {
        // emitter.offset = Vector2i(0, entityData.dimensions.y / 2);
        emitter.aabbHalf = ctx.entityData.size / 2;
    }

    entity.add(emitter);
}

void addComponentOrbit(ecs::Entity entity, const LoadContext& ctx) {
    Orbit orbit = entity.has<Orbit>() ? entity.get<Orbit>() : DefaultOrbit;

    tryReadFloat(ctx.values, "RotationsPerSecond", &orbit.rotationsPerSecond);

    const Vector2i entityDimensions = ctx.entityData.size;
    const Vector2i entityTrans = entity.get<Transform>().position;

    if (!ctx.values.contains("Target")) {
        print("Error: Orbit component requires a Target");
        return;
    }

    s32 shapeId = readInt(ctx.values, "Target");
    const auto& shapeObj = ctx.allObjects[ctx.idToIndex.at(shapeId).first];
    Vector2i otherDimensions = Vector2i::ZERO;
    bool isPoint = true;
    if (tryReadVector2i(shapeObj, "width", "height", &otherDimensions)) {
        isPoint = false;
    }
    const Vector2i otherTrans = getTransformFromMapPosition(readVector2i(shapeObj), otherDimensions, ctx.level, isPoint).position;

    orbit.radius = std::round((entityTrans - otherTrans).as<f32>().len());
    orbit.targetID = ctx.idToIndex.at(shapeId).second.id();

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

#include "ComponentFactory.h"

#include "CorradeOptional.h"
#include "ECS/TriggerZone.h"
#include "Game/Entities/Checkpoint.h"
#include "json.hpp"
#include "whalECS/src/ECS.h"

#include "Gfx/Texture.h"
#include "Physics/CollisionLayer.h"
#include "Settings.h"

#include "Map/Level.h"
#include "Map/Tiled.h"

#include "ECS/Collision.h"
#include "ECS/Draw.h"
#include "ECS/RailsControl.h"
#include "ECS/RigidBody.h"
#include "ECS/Systems/TagTrackers.h"
#include "ECS/Transform.h"
#include "ECS/Velocity.h"

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
T readVal(const nlohmann::json& data, std::string_view key) {
    MY_ASSERT(data.contains(key), whal_format("Missing key: {}", key).c_str());
    return data[key];
}

template <typename T>
void tryReadVal(const nlohmann::json& data, std::string_view key, T* dst) {
    if (data.contains(key)) {
        *dst = data[key];
    }
}

static NameToCreator<ComponentAdder> S_COMPONENT_ENTRIES[] = {
    {"Component_RailsControl", addComponentRailsControl},
    // {"Animator", addComponentAnimator},
    {"Component_ActorCollider", addComponentActorCollider},
    {"Component_SemiSolidCollider", addComponentSemiSolidCollider},
    {"Component_SolidCollider", addComponentSolidCollider},
    {"Component_RespawnTrigger", addComponentRespawnTrigger},
    {"Component_Draw", addComponentDraw},
    {"Component_Sprite_NoAnim", addComponentSprite},
    // {"Lifetime", addComponentLifetime},
    // {"PlayerControlRB", addComponentPlayerControlRB},
    // {"PlayerControlFree", addComponentPlayerControlFree},
    // {"Children", addComponentChildren},
    {"Component_Follow", addComponentFollow},
    {"Component_RigidBody", addComponentRigidBody},
    {"Component_Jumper", addComponentJumper},
    // {"Tags", addComponentTags},
    {"Component_Velocity", addComponentVelocity},
};

ComponentFactory::ComponentFactory() : Factory<ComponentAdder>("ComponentFactory") {
    initFactory(S_COMPONENT_ENTRIES);
}

void ComponentFactory::makeDefaultComponent(nlohmann::json property) {
    // who cares

    std::string componentName = property[KEY_NAME];
    ComponentAdder creatorFunc = nullptr;
    if (getEntryIndex(componentName.c_str(), &creatorFunc) == -1) {
        return;
    }

    if (componentName == "Component_RailsControl") {
        for (auto& member : property[KEY_MEMBERS]) {
            std::string memberName = member[KEY_NAME];
            if (memberName == "isCycle") {
                DefaultRailsControl.isCycle = member[KEY_VALUE];
            } else if (memberName == "speed") {
                DefaultRailsControl.speed = member[KEY_VALUE];
            } else if (memberName == "waitTime") {
                DefaultRailsControl.waitTime = member[KEY_VALUE];
            } else {
                print("Skipping member ", memberName, "for", componentName);
            }
        }

    } else if (componentName == "Component_ActorCollider") {
        Vector2i half;
        for (auto& member : property[KEY_MEMBERS]) {
            std::string memberName = member[KEY_NAME];
            if (memberName == "halflenTexelsX") {
                half.e[0] = member[KEY_VALUE];
            } else if (memberName == "halflenTexelsY") {
                half.e[1] = member[KEY_VALUE];
            } else if (memberName == "Material") {
                DefaultActorCollider.setMaterial(member[KEY_VALUE]);
            } else {
                print("Skipping member ", memberName, "for", componentName);
            }
        }
        half = Transform2D::texels(half.x(), half.y()).position;
        DefaultActorCollider.getShapeMutable().setHalf(half);

    } else if (componentName == "Component_SolidCollider") {
        Vector2i half;
        CollisionDir collisionDir;
        for (auto& member : property[KEY_MEMBERS]) {
            std::string memberName = member[KEY_NAME];
            if (memberName == "halflenTexelsX") {
                half.e[0] = member[KEY_VALUE];
            } else if (memberName == "halflenTexelsY") {
                half.e[1] = member[KEY_VALUE];
            } else if (memberName == "Material") {
                DefaultSolidCollider.setMaterial(member[KEY_VALUE]);
            } else if (memberName == "CollisionDir") {
                collisionDir = member[KEY_VALUE];
            } else {
                print("Skipping member ", memberName, "for", componentName);
            }
        }
        half = Transform2D::texels(half.x(), half.y()).position;
        DefaultSolidCollider.getShapeMutable().setHalf(half);
        DefaultSolidCollider.setCollisionDir(collisionDir);

    } else if (componentName == "Component_SemiSolidCollider") {
        Vector2i half;
        for (auto& member : property[KEY_MEMBERS]) {
            std::string memberName = member[KEY_NAME];
            if (memberName == "halflenTexelsX") {
                half.e[0] = member[KEY_VALUE];
            } else if (memberName == "halflenTexelsY") {
                half.e[1] = member[KEY_VALUE];
            } else if (memberName == "Material") {
                DefaultSemiSolidCollider.setMaterial(member[KEY_VALUE]);
            } else {
                print("Skipping member ", memberName, "for", componentName);
            }
        }
        half = Transform2D::texels(half.x(), half.y()).position;
        DefaultSemiSolidCollider.getShapeMutable().setHalf(half);

    } else if (componentName == "Component_Draw") {
        for (auto& member : property[KEY_MEMBERS]) {
            std::string memberName = member[KEY_NAME];
            if (memberName == "Color") {
                std::string hexString = member[KEY_VALUE];
                DefaultDraw.setColor(hexStringARGBToColor(hexString));
            } else {
                print("Skipping member ", memberName, "for", componentName);
            }
        }
    } else if (componentName == "Component_Sprite_NoAnim") {
        for (auto& member : property[KEY_MEMBERS]) {
            std::string memberName = member[KEY_NAME];
            if (memberName == "Color") {
                std::string hexString = member[KEY_VALUE];
                DefaultSprite.setColor(hexStringARGBToColor(hexString));
            } else if (memberName == "Sprite") {
                // do nothing
            } else {
                print("Skipping member ", memberName, "for", componentName);
            }
        }
    } else if (componentName == "Component_RigidBody") {
        for (auto& member : property[KEY_MEMBERS]) {
            std::string memberName = member[KEY_NAME];
            if (memberName == "momentumMultiplierX") {
                DefaultRigidBody.momentumMultiplier.e[0] = member[KEY_VALUE];
            } else if (memberName == "momentumMultiplierY") {
                DefaultRigidBody.momentumMultiplier.e[1] = member[KEY_VALUE];
            } else if (memberName == "frictionGround") {
                DefaultRigidBody.frictionMultiplier.e[0] = member[KEY_VALUE];
            } else if (memberName == "frictionAir") {
                DefaultRigidBody.frictionMultiplier.e[1] = member[KEY_VALUE];
            } else {
                print("Skipping member ", memberName, "for", componentName);
            }
        }
    } else if (componentName == "Component_Jumper") {
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
        for (auto& member : property[KEY_MEMBERS]) {
            std::string memberName = member[KEY_NAME];
            if (memberName == "velX") {
                DefaultVelocity.stable.e[0] = member[KEY_VALUE];
            } else if (memberName == "velY") {
                DefaultVelocity.stable.e[1] = member[KEY_VALUE];
            } else {
                print("Skipping member ", memberName, "for", componentName);
            }
        }
    } else if (componentName == "Component_Follow") {
        for (auto& member : property[KEY_MEMBERS]) {
            std::string memberName = member[KEY_NAME];
            if (memberName == "lookAheadX") {
                DefaultFollow.lookAheadTexels.e[0] = member[KEY_VALUE];
            } else if (memberName == "lookAheadY") {
                DefaultFollow.lookAheadTexels.e[1] = member[KEY_VALUE];
            } else if (memberName == "deadZoneX") {
                DefaultFollow.deadZoneTexels.e[0] = member[KEY_VALUE];
            } else if (memberName == "deadZoneY") {
                DefaultFollow.deadZoneTexels.e[1] = member[KEY_VALUE];
            } else if (memberName == "dampingX") {
                DefaultFollow.damping.e[0] = member[KEY_VALUE];
            } else if (memberName == "dampingY") {
                DefaultFollow.damping.e[1] = member[KEY_VALUE];
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
    } else {
        print("unhandled default component type: ", componentName);
    }
}

void addComponentVelocity(nlohmann::json& values, nlohmann::json& allObjects, std::unordered_map<s32, s32>& idToIndex, s32 thisId, ActiveLevel& level,
                          ecs::Entity entity, LayerData layerData) {
    Velocity velocity = ComponentFactory::DefaultVelocity;
    tryReadVector2f(values, "velX", "velY", &velocity.stable);

    entity.add(velocity);
}

void addComponentRailsControl(nlohmann::json& values, nlohmann::json& allObjects, std::unordered_map<s32, s32>& idToIndex, s32 thisId,
                              ActiveLevel& level, ecs::Entity entity, LayerData layerData) {
    std::vector<RailsControl::CheckPoint> checkpoints;
    Corrade::Containers::Optional<RailsControl::EndBehavior> endBehavior;
    if (values.contains("Checkpoints")) {
        s32 id = values["Checkpoints"];
        nlohmann::json checkPointObj = allObjects[idToIndex[id]];
        endBehavior = loadCheckpoints(checkPointObj, checkpoints, level);
    }

    RailsControl rails = ComponentFactory::DefaultRailsControl;
    rails.setCheckpoints(checkpoints, entity.get<Transform2D>());
    if (endBehavior) {
        rails.endBehavior = *endBehavior;
    }

    tryReadBool(values, "isCycle", &rails.isCycle);
    tryReadFloat(values, "speed", &rails.speed);
    tryReadFloat(values, "waitTime", &rails.waitTime);

    entity.add(rails);
}

void addComponentDraw(nlohmann::json& values, nlohmann::json& allObjects, std::unordered_map<s32, s32>& idToIndex, s32 thisId, ActiveLevel& level,
                      ecs::Entity entity, LayerData layerData) {
    s32 ix = idToIndex[thisId];
    Vector2i frameSizeTexels = {allObjects[ix]["width"], allObjects[ix]["height"]};

    Draw draw = ComponentFactory::DefaultDraw;
    draw.depth = layerData.depth;
    draw.setFrameSize(frameSizeTexels);

    // ARGB
    if (values.contains("Color")) {
        std::string hexcode = "#ffffffff";
        hexcode = values["Color"];
        Color color = hexStringARGBToColor(hexcode);
        draw.setColor(color);
    }

    entity.add(draw);
}

void addComponentSprite(nlohmann::json& values, nlohmann::json& allObjects, std::unordered_map<s32, s32>& idToIndex, s32 thisId, ActiveLevel& level,
                        ecs::Entity entity, LayerData layerData) {
    Sprite sprite = ComponentFactory::DefaultSprite;

    // ARGB
    if (values.contains("Color")) {
        std::string hexcode = "#ffffffff";
        hexcode = values["Color"];
        Color color = hexStringARGBToColor(hexcode);
        sprite.setColor(color);
    }

    std::string spritePath = "";
    if (values.contains("Sprite")) {
        spritePath = values["Sprite"];
        std::replace(spritePath.begin(), spritePath.end(), '\\', '/');
    }
    auto frameOpt = TextureManager::instance().getTextureAtlas(TEXNAME_SPRITE).getFrame(spritePath.c_str());
    if (frameOpt) {
        sprite.depth = layerData.depth;
        sprite.setFrame(*frameOpt);
        entity.add(sprite);
    } else {
        print("Coudn't find frame for sprite:", spritePath);
        // add draw instead
        s32 ix = idToIndex[thisId];
        Vector2i frameSizeTexels = {allObjects[ix]["width"], allObjects[ix]["height"]};
        Draw draw = ComponentFactory::DefaultDraw;
        draw.setFrameSize(frameSizeTexels);
        entity.add(draw);
    }
}

void addComponentActorCollider(nlohmann::json& values, nlohmann::json& allObjects, std::unordered_map<s32, s32>& idToIndex, s32 thisId,
                               ActiveLevel& level, ecs::Entity entity, LayerData layerData) {
    Collider actor = ComponentFactory::DefaultActorCollider;
    Vector2i halflenTexels = actor.getShape().getHalf() / PIXELS_PER_TEXEL;
    WorldMaterial material = actor.getMaterial();

    tryReadVector2i(values, "halflenTexelsX", "halflenTexelsY", &halflenTexels);
    tryReadVal(values, "Material", &material);

    entity.add(Collider(entity.get<Transform2D>(), halflenTexels * PIXELS_PER_TEXEL, CollisionLayer::Actor, material));
}

void addComponentSemiSolidCollider(nlohmann::json& values, nlohmann::json& allObjects, std::unordered_map<s32, s32>& idToIndex, s32 thisId,
                                   ActiveLevel& level, ecs::Entity entity, LayerData layerData) {
    Collider semi = ComponentFactory::DefaultSemiSolidCollider;
    Vector2i halflenTexels = semi.getShape().getHalf() / PIXELS_PER_TEXEL;
    WorldMaterial material = semi.getMaterial();

    tryReadVector2i(values, "halflenTexelsX", "halflenTexelsY", &halflenTexels);
    tryReadVal(values, "Material", &material);

    entity.add(Collider(entity.get<Transform2D>(), halflenTexels * PIXELS_PER_TEXEL, CollisionLayer::SemiSolid, material));
}

void addComponentSolidCollider(nlohmann::json& values, nlohmann::json& allObjects, std::unordered_map<s32, s32>& idToIndex, s32 thisId,
                               ActiveLevel& level, ecs::Entity entity, LayerData layerData) {
    Collider solid = ComponentFactory::DefaultSolidCollider;
    Vector2i halflenTexels = solid.getShape().getHalf() / PIXELS_PER_TEXEL;
    CollisionDir collisionDir = solid.getCollisionDir();
    WorldMaterial material = solid.getMaterial();

    tryReadVector2i(values, "halflenTexelsX", "halflenTexelsY", &halflenTexels);
    tryReadVal(values, "CollisionDir", &collisionDir);
    tryReadVal(values, "Material", &material);

    entity.add(Collider(entity.get<Transform2D>(), halflenTexels * PIXELS_PER_TEXEL, CollisionLayer::Solid, material, nullptr, collisionDir));
}

void addComponentRespawnTrigger(nlohmann::json& values, nlohmann::json& allObjects, std::unordered_map<s32, s32>& idToIndex, s32 thisId,
                                ActiveLevel& level, ecs::Entity entity, LayerData layerData) {
    auto object = allObjects[idToIndex[thisId]];
    auto positionTexels = readVector2i(object);
    Transform2D transform = getTransformFromMapPosition(positionTexels, {0, 0}, level, true);
    entity.set(transform);

    level.spawnPoints.push_back(transform.position);

    Vector2i zonePosition;
    Vector2i zoneDimensions;
    if (values.contains("Shape")) {
        s32 id = values["Shape"];
        nlohmann::json rect = allObjects[idToIndex[id]];
        zonePosition = readVector2i(rect);
        zoneDimensions = readVector2i(rect, "width", "height");

        zonePosition = getTransformFromMapPosition(zonePosition, zoneDimensions, level, false).position;
        zoneDimensions *= PIXELS_PER_TEXEL;  // do this *after* running that ^
    }

    Vector2i zoneCenter = zonePosition + Vector2i(0, zoneDimensions.y() / 2);
    entity.add(Trigger(AABB(zoneCenter, zoneDimensions / 2), CollisionLayer::TriggerActors, &onCheckpointEnter));
}

void addComponentFollow(nlohmann::json& values, nlohmann::json& allObjects, std::unordered_map<s32, s32>& idToIndex, s32 thisId, ActiveLevel& level,
                        ecs::Entity entity, LayerData layerData) {
    entity.add(loadFollowComponent(values, level));
}

void addComponentRigidBody(nlohmann::json& values, nlohmann::json& allObjects, std::unordered_map<s32, s32>& idToIndex, s32 thisId,
                           ActiveLevel& level, ecs::Entity entity, LayerData layerData) {
    RigidBody rb = ComponentFactory::DefaultRigidBody;

    tryReadVector2f(values, "momentumMultiplierX", "momentumMultiplierY", &rb.momentumMultiplier);
    tryReadVector2f(values, "frictionGround", "frictionAir", &rb.frictionMultiplier);

    entity.add(rb);
}

void addComponentJumper(nlohmann::json& values, nlohmann::json& allObjects, std::unordered_map<s32, s32>& idToIndex, s32 thisId, ActiveLevel& level,
                        ecs::Entity entity, LayerData layerData) {
    Jumper jumper = ComponentFactory::DefaultJumper;

    tryReadFloat(values, "jumpInitialVelocity", &jumper.jumpInitialVelocity);
    tryReadFloat(values, "jumpSecondsMax", &jumper.jumpSecondsMax);
    tryReadFloat(values, "coyoteTimeSecondsMax", &jumper.coyoteTimeSecondsMax);

    entity.add(jumper);
}

Follow loadFollowComponent(nlohmann::json& values, ActiveLevel& level) {
    Follow follow = ComponentFactory::DefaultFollow;
    if (values.contains("FollowTarget")) {
        // TODO compare to entities with Name component and follow first one which matches
        // std::string followTargetName = values["FollowTarget"];
        if (auto pPlayerSystem = System::world->getSystem<PlayerSystem>(); !pPlayerSystem->getEntitiesRef().empty()) {
            follow.targetEntityID = pPlayerSystem->first().id();
        }
    }

    tryReadVector2f(values, "dampingX", "dampingY", &follow.damping);
    tryReadVector2i(values, "deadZoneX", "deadZoneY", &follow.deadZoneTexels);
    tryReadVector2i(values, "lookAheadX", "lookAheadY", &follow.lookAheadTexels);
    tryReadVector2i(values, "boundsHalflenX", "boundsHalflenX", &follow.boundsXTexels);
    tryReadVector2i(values, "boundsHalflenY", "boundsHalflenY", &follow.boundsYTexels);

    // convert bounds from local half length to world coords
    Vector2i levelPosTexels = Vector2i(level.worldOffsetPixels.x() / PIXELS_PER_TEXEL, level.worldOffsetPixels.y() / PIXELS_PER_TEXEL) +
                              Vector2i(level.sizeTexels.x() / 2, level.sizeTexels.y() / 2);
    follow.boundsXTexels = {levelPosTexels.x() - follow.boundsXTexels.x(), levelPosTexels.x() + follow.boundsXTexels.x()};
    follow.boundsYTexels = {levelPosTexels.y() - follow.boundsYTexels.y(), levelPosTexels.y() + follow.boundsYTexels.y()};
    return follow;
}

RailsControl::EndBehavior loadCheckpoints(nlohmann::json& checkpointData, std::vector<RailsControl::CheckPoint>& dstCheckpoints, ActiveLevel& level) {
    // generic rewrite:
    const s32 parentX = readInt(checkpointData, "x");
    const s32 parentY = readInt(checkpointData, "y");
    const auto& properties = checkpointData["properties"];
    std::vector<RailsControl::Movement> moveProps;
    for (const auto& moveProperty : properties) {
        const s32 moveIx = moveProperty[KEY_VALUE];
        moveProps.push_back(static_cast<RailsControl::Movement>(moveIx));
    }

    bool isToStart = checkpointData.contains("polygon");
    std::string pathKey;
    if (isToStart) {
        pathKey = "polygon";
    } else {
        pathKey = "polyline";
    }
    size_t ix = 0;
    for (auto& point : checkpointData[pathKey]) {
        const s32 x = readInt(point, "x");
        const s32 y = readInt(point, "y");
        const Vector2i mapPos = {x + parentX, parentY + y};
        const Vector2i trans = getTransformFromMapPosition(mapPos, {0, 0}, level, true).position;

        RailsControl::Movement moveType;
        if (ix >= moveProps.size()) {
            std::string name = readString(checkpointData, KEY_NAME);
            print(name, " has ", moveProps.size(), " move type params but it has more points");
            moveType = RailsControl::Movement::LINEAR;
        } else {
            moveType = moveProps[ix];
        }
        RailsControl::CheckPoint chkPoint(trans, moveType);
        dstCheckpoints.push_back(chkPoint);

        ix++;
    }

    return isToStart ? RailsControl::EndBehavior::TO_START : RailsControl::EndBehavior::REVERSE;
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

void tryReadInt(const nlohmann::json& data, std::string_view key, s32* dst) {
    if (data.contains(key)) {
        *dst = data[key];
    }
}

void tryReadFloat(const nlohmann::json& data, std::string_view key, f32* dst) {
    if (data.contains(key)) {
        *dst = data[key];
    }
}

void tryReadVector2i(const nlohmann::json& data, std::string_view xKey, std::string_view yKey, Vector2i* dst) {
    if (data.contains(xKey)) {
        dst->e[0] = data[xKey];
    }
    if (data.contains(yKey)) {
        dst->e[1] = data[yKey];
    }
}

void tryReadVector2f(const nlohmann::json& data, std::string_view xKey, std::string_view yKey, Vector2f* dst) {
    if (data.contains(xKey)) {
        dst->e[0] = data[xKey];
    }
    if (data.contains(yKey)) {
        dst->e[1] = data[yKey];
    }
}

void tryReadBool(const nlohmann::json& data, std::string_view key, bool* dst) {
    if (data.contains(key)) {
        *dst = data[key];
    }
}

}  // namespace whal

#include "ComponentFactory.h"

#include "Gfx/Texture.h"
#include "Settings.h"

#include "json.hpp"
#include "whalECS/src/ECS.h"

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

namespace whal {

static const char* KEY_MEMBERS = "members";
static const char* KEY_NAME = "name";
static const char* KEY_VALUE = "value";

static NameToCreator<ComponentAdder> S_COMPONENT_ENTRIES[] = {
    {"Component_RailsControl", addComponentRailsControl},
    // {"Animator", addComponentAnimator},
    {"Component_ActorCollider", addComponentActorCollider},
    {"Component_SemiSolidCollider", addComponentSemiSolidCollider},
    {"Component_SolidCollider", addComponentSolidCollider},
    {"Component_Draw", addComponentDraw},
    {"Component_Sprite_NoAnim", addComponentSprite},
    // {"Lifetime", addComponentLifetime},
    // {"PlayerControlRB", addComponentPlayerControlRB},
    // {"PlayerControlFree", addComponentPlayerControlFree},
    // {"Children", addComponentChildren},
    {"Component_Follow", addComponentFollow},
    {"Component_RigidBody", addComponentRigidBody},
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
        DefaultActorCollider.getColliderMut().setHalf(half);

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
        DefaultSolidCollider.getColliderMut().setHalf(half);
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
        DefaultSemiSolidCollider.getColliderMut().setHalf(half);

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
            if (memberName == "coyoteTimeSecondsMax") {
                DefaultRigidBody.coyoteTimeSecondsMax = member[KEY_VALUE];
            } else if (memberName == "jumpInitialVelocity") {
                DefaultRigidBody.jumpInitialVelocity = member[KEY_VALUE];
            } else if (memberName == "jumpSecondsMax") {
                DefaultRigidBody.jumpSecondsMax = member[KEY_VALUE];
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

    if (values.contains("velX")) {
        velocity.stable.e[0] = values["velX"];
    }
    if (values.contains("velY")) {
        velocity.stable.e[1] = values["velY"];
    }

    entity.add(velocity);
}

void addComponentRailsControl(nlohmann::json& values, nlohmann::json& allObjects, std::unordered_map<s32, s32>& idToIndex, s32 thisId,
                              ActiveLevel& level, ecs::Entity entity, LayerData layerData) {
    std::vector<RailsControl::CheckPoint> checkpoints;
    std::optional<RailsControl::EndBehavior> endBehavior;
    if (values.contains("Checkpoints")) {
        s32 id = values["Checkpoints"];
        nlohmann::json checkPointObj = allObjects[idToIndex[id]];
        endBehavior = loadCheckpoints(checkPointObj, checkpoints, level);
    }

    RailsControl rails = ComponentFactory::DefaultRailsControl;
    rails.setCheckpoints(checkpoints, entity.get<Transform2D>());
    if (endBehavior) {
        rails.endBehavior = endBehavior.value();
    }

    if (values.contains("isCycle")) {
        rails.isCycle = values["isCycle"];
    }
    if (values.contains("speed")) {
        rails.speed = values["speed"];
    }
    if (values.contains("waitTime")) {
        rails.waitTime = values["waitTime"];
    }

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
        sprite.setFrame(frameOpt.value());
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
    ActorCollider actor = ComponentFactory::DefaultActorCollider;
    Vector2i halflenTexels = actor.getCollider().getHalf() / PIXELS_PER_TEXEL;
    WorldMaterial material = actor.getMaterial();

    if (values.contains("halflenTexelsX")) {
        halflenTexels.e[0] = values["halflenTexelsX"];
    }
    if (values.contains("halflenTexelsY")) {
        halflenTexels.e[1] = values["halflenTexelsY"];
    }
    if (values.contains("Material")) {
        material = values["Material"];
    }

    entity.add(ActorCollider(entity.get<Transform2D>(), halflenTexels * PIXELS_PER_TEXEL, material));
}

void addComponentSemiSolidCollider(nlohmann::json& values, nlohmann::json& allObjects, std::unordered_map<s32, s32>& idToIndex, s32 thisId,
                                   ActiveLevel& level, ecs::Entity entity, LayerData layerData) {
    SemiSolidCollider semi = ComponentFactory::DefaultSemiSolidCollider;
    Vector2i halflenTexels = semi.getCollider().getHalf() / PIXELS_PER_TEXEL;
    WorldMaterial material = semi.getMaterial();

    if (values.contains("halflenTexelsX")) {
        halflenTexels.e[0] = values["halflenTexelsX"];
    }
    if (values.contains("halflenTexelsY")) {
        halflenTexels.e[1] = values["halflenTexelsY"];
    }
    if (values.contains("Material")) {
        material = values["Material"];
    }

    entity.add(SemiSolidCollider(entity.get<Transform2D>(), halflenTexels * PIXELS_PER_TEXEL, material));
}

void addComponentSolidCollider(nlohmann::json& values, nlohmann::json& allObjects, std::unordered_map<s32, s32>& idToIndex, s32 thisId,
                               ActiveLevel& level, ecs::Entity entity, LayerData layerData) {
    SolidCollider solid = ComponentFactory::DefaultSolidCollider;
    Vector2i halflenTexels = solid.getCollider().getHalf() / PIXELS_PER_TEXEL;
    CollisionDir collisionDir = solid.getCollisionDir();
    WorldMaterial material = solid.getMaterial();

    if (values.contains("halflenTexelsX")) {
        halflenTexels.e[0] = values["halflenTexelsX"];
    }
    if (values.contains("halflenTexelsY")) {
        halflenTexels.e[1] = values["halflenTexelsY"];
    }
    if (values.contains("CollisionDir")) {
        collisionDir = values["CollisionDir"];
    }
    if (values.contains("Material")) {
        material = values["Material"];
    }

    entity.add(SolidCollider(entity.get<Transform2D>(), halflenTexels * PIXELS_PER_TEXEL, material, nullptr, collisionDir));
}

void addComponentFollow(nlohmann::json& values, nlohmann::json& allObjects, std::unordered_map<s32, s32>& idToIndex, s32 thisId, ActiveLevel& level,
                        ecs::Entity entity, LayerData layerData) {
    entity.add(loadFollowComponent(values, level));
}

void addComponentRigidBody(nlohmann::json& values, nlohmann::json& allObjects, std::unordered_map<s32, s32>& idToIndex, s32 thisId,
                           ActiveLevel& level, ecs::Entity entity, LayerData layerData) {
    RigidBody rb = ComponentFactory::DefaultRigidBody;

    if (values.contains("jumpInitialVelocity")) {
        rb.jumpInitialVelocity = values["jumpInitialVelocity"];
    }
    if (values.contains("jumpSecondsMax")) {
        rb.jumpSecondsMax = values["jumpSecondsMax"];
    }
    if (values.contains("coyoteTimeSecondsMax")) {
        rb.coyoteTimeSecondsMax = values["coyoteTimeSecondsMax"];
    }

    entity.add(rb);
}

Follow loadFollowComponent(nlohmann::json& values, ActiveLevel& level) {
    Follow follow = ComponentFactory::DefaultFollow;
    if (values.contains("FollowTarget")) {
        // TODO compare to entities with Name component and follow first one which matches
        // std::string followTargetName = values["FollowTarget"];
        follow.targetEntity = PlayerSystem::instance()->first();
    }
    if (values.contains("dampingX")) {
        follow.damping.e[0] = values["dampingX"];
    }
    if (values.contains("dampingY")) {
        follow.damping.e[1] = values["dampingY"];
    }
    if (values.contains("deadZoneX")) {
        follow.deadZoneTexels.e[0] = values["deadZoneX"];
    }
    if (values.contains("deadZoneY")) {
        follow.deadZoneTexels.e[1] = values["deadZoneY"];
    }
    if (values.contains("lookAheadX")) {
        follow.lookAheadTexels.e[0] = values["lookAheadX"];
    }
    if (values.contains("lookAheadY")) {
        follow.lookAheadTexels.e[1] = values["lookAheadY"];
    }
    if (values.contains("boundsHalflenX")) {
        follow.boundsXTexels = Vector2i(values["boundsHalflenX"], values["boundsHalflenX"]);
    }
    if (values.contains("boundsHalflenY")) {
        follow.boundsYTexels = Vector2i(values["boundsHalflenY"], values["boundsHalflenY"]);
    }

    // convert bounds from local half length to world coords
    Vector2i levelPosTexels = Vector2i(level.worldOffsetPixels.x() / PIXELS_PER_TEXEL, level.worldOffsetPixels.y() / PIXELS_PER_TEXEL) +
                              Vector2i(level.sizeTexels.x() / 2, level.sizeTexels.y() / 2);
    follow.boundsXTexels = {levelPosTexels.x() - follow.boundsXTexels.x(), levelPosTexels.x() + follow.boundsXTexels.x()};
    follow.boundsYTexels = {levelPosTexels.y() - follow.boundsYTexels.y(), levelPosTexels.y() + follow.boundsYTexels.y()};
    return follow;
}

RailsControl::EndBehavior loadCheckpoints(nlohmann::json& checkpointData, std::vector<RailsControl::CheckPoint>& dstCheckpoints, ActiveLevel& level) {
    // generic rewrite:
    s32 parentX = checkpointData["x"];
    s32 parentY = checkpointData["y"];
    auto& properties = checkpointData["properties"];
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
        const s32 x = point["x"];
        const s32 y = point["y"];
        const Vector2i mapPos = {x + parentX, parentY + y};
        const Vector2i trans = getTransformFromMapPosition(mapPos, {0, 0}, level, true).position;

        RailsControl::Movement moveType;
        if (ix >= moveProps.size()) {
            std::string name = checkpointData[KEY_NAME];
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

}  // namespace whal

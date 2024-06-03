#pragma once

#include <unordered_map>

#include "json_fwd.hpp"

#include "ECS/Collision.h"
#include "ECS/Draw.h"
#include "ECS/RailsControl.h"
#include "ECS/Relationships.h"
#include "ECS/RigidBody.h"
#include "ECS/Velocity.h"

#include "Map/Level.h"
#include "Util/Factory.h"

namespace whal {

namespace ecs {
class Entity;
}
struct LayerData;

// there is no base component class, so I'll pass the entity to the creation function instead of returning a component
using ComponentAdder = void (*)(nlohmann::json&, nlohmann::json&, std::unordered_map<s32, s32>&, s32, ActiveLevel&, ecs::Entity, LayerData layerData);
class ComponentFactory : public Factory<ComponentAdder> {
public:
    ComponentFactory();

    void makeDefaultComponent(nlohmann::json property);

    inline static Velocity DefaultVelocity;
    inline static RailsControl DefaultRailsControl;
    inline static ActorCollider DefaultActorCollider;
    inline static SemiSolidCollider DefaultSemiSolidCollider;
    inline static SolidCollider DefaultSolidCollider;
    inline static RigidBody DefaultRigidBody;
    inline static Draw DefaultDraw;
    inline static Sprite DefaultSprite;
    inline static Follow DefaultFollow;
};

void addComponentVelocity(nlohmann::json& values, nlohmann::json& allObjects, std::unordered_map<s32, s32>& idToIndex, s32 thisId, ActiveLevel& level,
                          ecs::Entity entity, LayerData layerData);
void addComponentRailsControl(nlohmann::json& values, nlohmann::json& allObjects, std::unordered_map<s32, s32>& idToIndex, s32 thisId,
                              ActiveLevel& level, ecs::Entity entity, LayerData layerData);
void addComponentActorCollider(nlohmann::json& values, nlohmann::json& allObjects, std::unordered_map<s32, s32>& idToIndex, s32 thisId,
                               ActiveLevel& level, ecs::Entity entity, LayerData layerData);
void addComponentSemiSolidCollider(nlohmann::json& values, nlohmann::json& allObjects, std::unordered_map<s32, s32>& idToIndex, s32 thisId,
                                   ActiveLevel& level, ecs::Entity entity, LayerData layerData);
void addComponentSolidCollider(nlohmann::json& values, nlohmann::json& allObjects, std::unordered_map<s32, s32>& idToIndex, s32 thisId,
                               ActiveLevel& level, ecs::Entity entity, LayerData layerData);
void addComponentRigidBody(nlohmann::json& values, nlohmann::json& allObjects, std::unordered_map<s32, s32>& idToIndex, s32 thisId,
                           ActiveLevel& level, ecs::Entity entity, LayerData layerData);
void addComponentDraw(nlohmann::json& values, nlohmann::json& allObjects, std::unordered_map<s32, s32>& idToIndex, s32 thisId, ActiveLevel& level,
                      ecs::Entity entity, LayerData layerData);
void addComponentSprite(nlohmann::json& values, nlohmann::json& allObjects, std::unordered_map<s32, s32>& idToIndex, s32 thisId, ActiveLevel& level,
                        ecs::Entity entity, LayerData layerData);
// void addComponentAnimator(nlohmann::json& data, ecs::Entity entity);
// void addComponentLifetime(nlohmann::json& data, ecs::Entity entity);
// void addComponentPlayerControlRB(nlohmann::json& data, ecs::Entity entity);
// void addComponentPlayerControlFree(nlohmann::json& data, ecs::Entity entity);
// void addComponentChildren(nlohmann::json& data, ecs::Entity entity);
void addComponentFollow(nlohmann::json& values, nlohmann::json& allObjects, std::unordered_map<s32, s32>& idToIndex, s32 thisId, ActiveLevel& level,
                        ecs::Entity entity, LayerData layerData);
// void addComponentTags(nlohmann::json& data, ecs::Entity entity);

Follow loadFollowComponent(nlohmann::json& values, ActiveLevel& level);
void loadCheckpoints(nlohmann::json& checkpointData, std::vector<RailsControl::CheckPoint>& dstCheckpoints, ActiveLevel& level);

}  // namespace whal

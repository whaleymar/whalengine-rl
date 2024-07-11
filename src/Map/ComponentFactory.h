#pragma once

#include <unordered_map>

#include "Game/Components/Switch.h"
#include "json_fwd.hpp"

#include "ECS/Collision.h"
#include "ECS/Draw.h"
#include "ECS/Lifetime.h"
#include "ECS/Light.h"
#include "ECS/ParticleEmitter.h"
#include "ECS/PlayerControl.h"
#include "ECS/RailsControl.h"
#include "ECS/Relationships.h"
#include "ECS/RigidBody.h"
#include "ECS/TriggerZone.h"
#include "ECS/Velocity.h"

#include "Util/Factory.h"

namespace whal {

namespace ecs {
class Entity;
}
struct LayerData;
struct ActiveLevel;
struct EntityMapData;

// there is no base component class, so I'll pass the entity to the creation function instead of returning a component
using ComponentAdder = void (*)(const nlohmann::json&, const nlohmann::json&, const std::unordered_map<s32, std::pair<s32, ecs::Entity>>&,
                                EntityMapData, ActiveLevel&, ecs::Entity, LayerData layerData);
class ComponentFactory : public Factory<ComponentAdder> {
public:
    ComponentFactory();

    void makeDefaultComponent(const nlohmann::json& property);

    inline static Velocity DefaultVelocity;
    inline static RailsControl DefaultRailsControl;
    inline static Collider DefaultCollider;
    inline static Trigger DefaultTrigger;
    inline static RigidBody DefaultRigidBody;
    inline static PlayerControl DefaultPlayerControl;
    inline static Jumper DefaultJumper;
    inline static Draw DefaultDraw;
    inline static Sprite DefaultSprite;
    inline static FadeOut DefaultFadeout;
    inline static Follow DefaultFollow;
    inline static Attach DefaultAttach;
    inline static PointLight DefaultPointLight;
    inline static Radiance DefaultRadiance;
    inline static Lifetime DefaultLifeTime;
    inline static SwitchGate DefaultSwitchGate;
    inline static DrawText DefaultDrawText;
    inline static ParticleEmitter DefaultParticleEmitter;
};

void addTagComponents(const nlohmann::json& values, const nlohmann::json& allObjects,
                      const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, ActiveLevel& level,
                      ecs::Entity entity, LayerData layerData);
void addComponentVelocity(const nlohmann::json& values, const nlohmann::json& allObjects,
                          const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, ActiveLevel& level,
                          ecs::Entity entity, LayerData layerData);
void addComponentRailsControl(const nlohmann::json& values, const nlohmann::json& allObjects,
                              const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, ActiveLevel& level,
                              ecs::Entity entity, LayerData layerData);
void addComponentCollider(const nlohmann::json& values, const nlohmann::json& allObjects,
                          const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, ActiveLevel& level,
                          ecs::Entity entity, LayerData layerData);
void addComponentTrigger(const nlohmann::json& values, const nlohmann::json& allObjects,
                         const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, ActiveLevel& level,
                         ecs::Entity entity, LayerData layerData);
void addComponentRigidBody(const nlohmann::json& values, const nlohmann::json& allObjects,
                           const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, ActiveLevel& level,
                           ecs::Entity entity, LayerData layerData);
void addComponentPlayerControl(const nlohmann::json& values, const nlohmann::json& allObjects,
                               const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, ActiveLevel& level,
                               ecs::Entity entity, LayerData layerData);
void addComponentJumper(const nlohmann::json& values, const nlohmann::json& allObjects,
                        const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, ActiveLevel& level,
                        ecs::Entity entity, LayerData layerData);
void addComponentDraw(const nlohmann::json& values, const nlohmann::json& allObjects,
                      const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, ActiveLevel& level,
                      ecs::Entity entity, LayerData layerData);
void addComponentSprite(const nlohmann::json& values, const nlohmann::json& allObjects,
                        const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, ActiveLevel& level,
                        ecs::Entity entity, LayerData layerData);
void addComponentFadeout(const nlohmann::json& values, const nlohmann::json& allObjects,
                         const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, ActiveLevel& level,
                         ecs::Entity entity, LayerData layerData);
void addComponentLight(const nlohmann::json& values, const nlohmann::json& allObjects,
                       const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, ActiveLevel& level,
                       ecs::Entity entity, LayerData layerData);
void addComponentRadiance(const nlohmann::json& values, const nlohmann::json& allObjects,
                          const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, ActiveLevel& level,
                          ecs::Entity entity, LayerData layerData);
void addComponentLifetime(const nlohmann::json& values, const nlohmann::json& allObjects,
                          const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, ActiveLevel& level,
                          ecs::Entity entity, LayerData layerData);
void addComponentFollow(const nlohmann::json& values, const nlohmann::json& allObjects,
                        const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, ActiveLevel& level,
                        ecs::Entity entity, LayerData layerData);
void addComponentAttach(const nlohmann::json& values, const nlohmann::json& allObjects,
                        const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, EntityMapData entityData, ActiveLevel& level,
                        ecs::Entity entity, LayerData layerData);

Follow loadFollowComponent(const nlohmann::json& values, ActiveLevel& level);
bool loadCheckpoints(const nlohmann::json& checkpointData, std::vector<RailsControl::CheckPoint>& dstCheckpoints, ActiveLevel& level);

// Utility Functions

s32 readInt(const nlohmann::json& json, std::string_view key);
s32 readFloat(const nlohmann::json& json, std::string_view key);
Vector2i readVector2i(const nlohmann::json& json, const char* xKey = "x", const char* yKey = "y");
bool readBool(const nlohmann::json& data, std::string_view key);
std::string readString(const nlohmann::json& json, std::string_view key);

bool tryReadInt(const nlohmann::json& data, std::string_view key, s32* dst);
bool tryReadFloat(const nlohmann::json& data, std::string_view key, f32* dst);
bool tryReadVector2i(const nlohmann::json& data, std::string_view xKey, std::string_view yKey, Vector2i* dst);
bool tryReadVector2f(const nlohmann::json& data, std::string_view xKey, std::string_view yKey, Vector2f* dst);
bool tryReadBool(const nlohmann::json& data, std::string_view key, bool* dst);
bool tryReadString(const nlohmann::json& data, std::string_view key, std::string* dst);

}  // namespace whal

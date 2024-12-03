#pragma once

#include <unordered_map>

#include "Util/Print.h"
#include "json_fwd.hpp"

#include "Util/DynamicFactory.h"
#include "Util/ISerialize.h"
#include "Util/Vector.h"

#define REGISTER_COMPONENT(component) static inline const bool S_INITFLAG_##component = component::S_IS_REGISTERED;

namespace whal {

namespace ecs {
class Entity;
}
struct LayerData;
struct ActiveLevel;
struct EntityMapData;
struct Follow;

// passed as const reference when loading components
struct LoadContext {
    const nlohmann::json& values;  // should rename this to `componentData` or something
    const nlohmann::json& allObjects;
    const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex;
    const EntityMapData& entityData;
    const ActiveLevel& level;
    const LayerData& layerData;
    bool isTiledData = false;
};

// there is no base component class, so I'll pass the entity to the creation function instead of returning a component
// using ComponentAdder = void (*)(const nlohmann::json&, const nlohmann::json&, const std::unordered_map<s32, std::pair<s32, ecs::Entity>>&,
// EntityMapData, const ActiveLevel&, ecs::Entity, LayerData layerData);
using ComponentAdder = void (*)(ecs::Entity, const LoadContext&);
class ComponentFactory : public DynamicFactory<ComponentAdder> {
public:
    ComponentFactory();

    void makeDefaultComponent(const nlohmann::json& property);
};

Follow loadFollowComponent(const nlohmann::json& values, const ActiveLevel& level);

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

/////////////////////////////////////////////////////////////////////
// NEW IMPLEMENTATION
/////////////////////////////////////////////////////////////////////

// Tiled Data types I need to handle:
// int, float, string, bool
//
struct ComponentFactoryNew : SerializeFactory<ComponentFactoryNew, MAX_COMPONENTS> {
    template <typename T>
    static void DefaultLoadImpl(ecs::Entity entity, void* data) {
        print("Running ComponentFactoryNew::DefaultLoadImpl");
        const LoadContext& ctx = *static_cast<LoadContext*>(data);
        if (ctx.isTiledData) {
        }
    }

    template <typename T>
    static void* DefaultSaveImpl(ecs::Entity entity) {
        print("Running ComponentFactoryNew::DefaultSaveImpl");
        return nullptr;
    }
};

}  // namespace whal

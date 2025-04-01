#pragma once

#include <string_view>
#include "TypeName.h"
#include "Util/Print.h"
#include "Util/Types.h"
#include "json_fwd.hpp"

template <typename T>
struct Vector2;
typedef Vector2<f32> Vector2f;
typedef Vector2<s32> Vector2i;

namespace whal {

struct Color;
enum class Depth : u8;
class Shape;
struct LoadContext;
struct TileMapEntityDescriptor;

namespace ecs {
class Entity;
}

// base reader methods:
s32 readInt(const nlohmann::json& json, std::string_view key);
s32 readFloat(const nlohmann::json& json, std::string_view key);
Vector2i readVector2i(const nlohmann::json& json, const char* xKey = "x", const char* yKey = "y");
Vector2f readVector2f(const nlohmann::json& json, const char* xKey = "x", const char* yKey = "y");
bool readBool(const nlohmann::json& data, std::string_view key);
std::string readString(const nlohmann::json& json, std::string_view key);
TileMapEntityDescriptor readEntity(const nlohmann::json& json, std::string_view key);
Color readColor(const std::string& hexString);
Depth readDepth(const std::string& depthString);

// TODO need to unify API
Shape readShape(const LoadContext& ctx, std::string_view key, Vector2i* dstOffset = nullptr);
Shape getDefaultShape(const LoadContext& ctx);  // uses shape of Tiled object instead of a Property
bool tryReadShape(const LoadContext& ctx, std::string_view key, Shape* dst, Vector2i* dstOffset = nullptr);
Shape readShapeOrDefault(const LoadContext& ctx, std::string_view key,
                         Vector2i* dstOffset = nullptr);  // tries to get a custom shape, returns default shape if none present

template <typename T>
bool tryRead(const nlohmann::json& data, std::string_view key, T* dst) {
    print("tryRead not implemented for", type_of<T>());
    return false;
}

template <>
bool tryRead(const nlohmann::json& data, std::string_view key, s32* dst);

template <>
bool tryRead(const nlohmann::json& data, std::string_view key, f32* dst);

template <>
bool tryRead(const nlohmann::json& data, std::string_view key, bool* dst);

template <>
bool tryRead(const nlohmann::json& data, std::string_view key, std::string* dst);

template <>
bool tryRead(const nlohmann::json& data, std::string_view key, Color* dst);

template <>
bool tryRead(const nlohmann::json& data, std::string_view key, Depth* dst);

template <>
bool tryRead(const nlohmann::json& data, std::string_view key, Vector2i* dst);

template <>
bool tryRead(const nlohmann::json& data, std::string_view key, Vector2f* dst);

template <>
bool tryRead(const nlohmann::json& data, std::string_view key, TileMapEntityDescriptor* dst);

bool tryRead(const nlohmann::json& data, std::string_view xKey, std::string_view yKey, Vector2i* dst);
bool tryRead(const nlohmann::json& data, std::string_view xKey, std::string_view yKey, Vector2f* dst);

}  // namespace whal

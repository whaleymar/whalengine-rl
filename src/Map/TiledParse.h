#pragma once

#include "Util/JsonDoc.h"
#include "Util/Types.h"

class JsonValue;

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
s32 readInt(const JsonValue json, const char* key);
s32 readFloat(const JsonValue json, const char* key);
Vector2i readVector2i(const JsonValue json, const char* xKey = "x", const char* yKey = "y");
Vector2f readVector2f(const JsonValue json, const char* xKey = "x", const char* yKey = "y");
bool readBool(const JsonValue data, const char* key);
std::string readString(const JsonValue json, const char* key);
Color readColor(const std::string& hexString);
Depth readDepth(const std::string& depthString);

// TODO need to unify API
Shape readShape(const LoadContext& ctx, const char* key, Vector2i* dstOffset = nullptr);
Shape getDefaultShape(const LoadContext& ctx);  // uses shape of Tiled object instead of a Property
bool tryReadShape(const LoadContext& ctx, const char* key, Shape* dst, Vector2i* dstOffset = nullptr);
Shape readShapeOrDefault(const LoadContext& ctx, const char* key,
                         Vector2i* dstOffset = nullptr);  // tries to get a custom shape, returns default shape if none present

// template required for reflection implementations to compile (they need this stubbed placeholder)
template <typename T>
bool tryRead(const JsonValue data, const char* key, T* dst) {
    // not implemented
    return false;
}

template <>
bool tryRead(const JsonValue data, const char* key, s32* dst);

template <>
bool tryRead(const JsonValue data, const char* key, f32* dst);

template <>
bool tryRead(const JsonValue data, const char* key, bool* dst);

template <>
bool tryRead(const JsonValue data, const char* key, std::string* dst);

template <>
bool tryRead(const JsonValue data, const char* key, Color* dst);

template <>
bool tryRead(const JsonValue data, const char* key, Depth* dst);

template <>
bool tryRead(const JsonValue data, const char* key, Vector2i* dst);

template <>
bool tryRead(const JsonValue data, const char* key, Vector2f* dst);

template <>
bool tryRead(const JsonValue data, const char* key, TileMapEntityDescriptor* dst);

bool tryRead(const JsonValue data, const char* xKey, const char* yKey, Vector2i* dst);
bool tryRead(const JsonValue data, const char* xKey, const char* yKey, Vector2f* dst);

}  // namespace whal

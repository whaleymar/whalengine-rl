#include "TiledParse.h"

#include "Map/Tiled.h"
#include "Util/DebugUtil.h"
#include "Util/Vector.h"
#include "json.hpp"
#include "rfl/enums.hpp"

namespace whal {

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

Vector2f readVector2f(const nlohmann::json& data, const char* xKey, const char* yKey) {
    DBG_ASSERT(data.contains(xKey), data.contains(yKey) && whal_format("Missing keys: {} & {}", xKey, yKey).c_str());
    return Vector2f(data[xKey], data[yKey]);
}

bool readBool(const nlohmann::json& data, std::string_view key) {
    DBG_ASSERT(data.contains(key), whal_format("Missing key: {}", key).c_str());
    return data[key];
}

std::string readString(const nlohmann::json& data, std::string_view key) {
    DBG_ASSERT(data.contains(key), whal_format("Missing key: {}", key).c_str());
    return data[key];
}

Color readColor(const std::string& hexString) {
    s32 r, g, b, a;
    // format is "#aarrggbb"
    std::istringstream(hexString.substr(1, 2)) >> std::hex >> a;
    std::istringstream(hexString.substr(3, 2)) >> std::hex >> r;
    std::istringstream(hexString.substr(5, 2)) >> std::hex >> g;
    std::istringstream(hexString.substr(7, 2)) >> std::hex >> b;

    return Color::fromRGB(r, g, b, a);
}

Depth readDepth(const std::string& depthString) {
    return rfl::string_to_enum<Depth>(depthString).value();
}

Shape readShape(const LoadContext& ctx, ecs::Entity entity, std::string_view key, Vector2i* dstOffset) {
    DBG_ASSERT(ctx.values.contains(key), whal_format("Missing key: {}", key).c_str());

    s32 shapeId = readInt(ctx.values, key);
    // calc distance between this object and Shape for the offset
    const auto& shapeObj = ctx.allObjects[ctx.idToIndex.at(shapeId).first];
    const Vector2i otherDims = readVector2i(shapeObj, "width", "height");
    const Vector2i halflen = otherDims / 2;
    const Vector2i thisTrans = getTransformFromMapPosition(ctx.entityData.position, ctx.entityData.size, ctx.level, ctx.entityData.isPoint).position;

    const Vector2i otherTrans = getTransformFromMapPosition(readVector2i(shapeObj), otherDims, ctx.level, false).position;
    const auto offset = otherTrans - thisTrans;

    if (dstOffset != nullptr) {
        *dstOffset = offset;
    }

    Shape shape;

    if (shapeObj.contains("ellipse")) {
        const s32 radius = std::max(halflen.x, halflen.y);
        shape = Circle(entity.get<Transform>(), radius, offset);
    } else {
        // no field for rectangle, it's the default
        shape = AABB(entity.get<Transform>(), halflen, offset);
    }

    return shape;
}

Shape getDefaultShape(const LoadContext& ctx, ecs::Entity entity) {
    // would be used for a default ellipse getter (if I need one):
    // const s32 radius = std::max(ctx.entityData.size.x, ctx.entityData.size.y) / 2;
    return AABB(entity.get<Transform>(), ctx.entityData.size / 2, Vector2i());
}

Shape readShapeOrDefault(const LoadContext& ctx, ecs::Entity entity, std::string_view key, Vector2i* dstOffset) {
    Shape shape;
    if (tryReadShape(ctx, entity, key, &shape, dstOffset)) {
        return shape;
    }
    return getDefaultShape(ctx, entity);
}

template <>
bool tryRead(const nlohmann::json& data, std::string_view key, s32* dst) {
    if (data.contains(key)) {
        *dst = data[key];
        return true;
    }
    return false;
}

template <>
bool tryRead(const nlohmann::json& data, std::string_view key, f32* dst) {
    if (data.contains(key)) {
        *dst = data[key];
        return true;
    }
    return false;
}

template <>
bool tryRead(const nlohmann::json& data, std::string_view key, bool* dst) {
    if (data.contains(key)) {
        *dst = data[key];
        return true;
    }
    return false;
}

template <>
bool tryRead(const nlohmann::json& data, std::string_view key, std::string* dst) {
    if (data.contains(key)) {
        *dst = data[key];
        return true;
    }
    return false;
}

template <>
bool tryRead(const nlohmann::json& data, std::string_view key, Color* dst) {
    if (data.contains(key)) {
        std::string hexString = readString(data, key);
        *dst = readColor(hexString);
        return true;
    }
    return false;
}

template <>
bool tryRead(const nlohmann::json& data, std::string_view key, Depth* dst) {
    if (data.contains(key)) {
        std::string str = readString(data, key);
        *dst = readDepth(str);
        return true;
    }
    return false;
}

template <>
bool tryRead(const nlohmann::json& data, std::string_view key, Vector2i* dst) {
    if (data.contains(key)) {
        tryRead(data[key], "x", &dst->x);
        tryRead(data[key], "y", &dst->y);
        return true;
    }
    return false;
}

template <>
bool tryRead(const nlohmann::json& data, std::string_view key, Vector2f* dst) {
    if (data.contains(key)) {
        tryRead(data[key], "x", &dst->x);
        tryRead(data[key], "y", &dst->y);
        return true;
    }
    return false;
}

bool tryRead(const nlohmann::json& data, std::string_view xKey, std::string_view yKey, Vector2f* dst) {
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

bool tryRead(const nlohmann::json& data, std::string_view xKey, std::string_view yKey, Vector2i* dst) {
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

bool tryReadShape(const LoadContext& ctx, ecs::Entity entity, std::string_view key, Shape* dst, Vector2i* dstOffset) {
    if (ctx.values.contains(key)) {
        *dst = readShape(ctx, entity, key, dstOffset);
        return true;
    }
    return false;
}

}  // namespace whal

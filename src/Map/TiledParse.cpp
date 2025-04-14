#include "TiledParse.h"

#include "Components/Map.h"
#include "Components/Tags.h"
#include "Map/ComponentFactory.h"
#include "Map/Tiled.h"
#include "Physics/Shapes.h"
#include "Util/DebugUtil.h"
#include "Util/JsonDoc.h"
#include "Util/MathUtil.h"
#include "Util/Vector.h"
#include "rfl/enums.hpp"

namespace whal {

s32 readInt(const JsonValue data, const char* key) {
    return data[key].getInt();
}

s32 readFloat(const JsonValue data, const char* key) {
    return data[key].getNumber();
}

Vector2i readVector2i(const JsonValue data, const char* xKey, const char* yKey) {
    return Vector2i(data[xKey].getInt(), data[yKey].getInt());
}

Vector2f readVector2f(const JsonValue data, const char* xKey, const char* yKey) {
    return Vector2f(data[xKey].getNumber(), data[yKey].getNumber());
}

bool readBool(const JsonValue data, const char* key) {
    return data[key].getBool();
}

std::string readString(const JsonValue data, const char* key) {
    return data[key].getString();
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

Shape readShape(const LoadContext& ctx, const char* key, Vector2i* dstOffset) {
    DBG_ASSERT(ctx.values.contains(key), whal_format("Missing key: {}", key).c_str());

    s32 shapeId = readInt(ctx.values, key);
    // calc distance between this object and Shape for the offset
    const JsonValue shapeObj = ctx.allObjects[ctx.idToIndex.at(shapeId).first];
    const Vector2i otherDims = readVector2i(shapeObj, "width", "height");
    const Vector2i halflen = otherDims / 2;
    const Vector2i thisTrans = getMapTransform(ctx.entityData.position, ctx.entityData.size, ctx.parent).positionPx;

    const Vector2i otherTrans = getMapTransform(readVector2i(shapeObj), otherDims, ctx.parent).positionPx;
    const Vector2i offset = otherTrans - thisTrans;

    if (dstOffset != nullptr) {
        *dstOffset = offset;
    }

    Shape shape;
    if (shapeObj.contains("ellipse")) {
        const s32 radius = std::max(halflen.x, halflen.y);
        shape = Circle(ctx.self.get<Transform>(), radius, offset);
    } else {
        // no field for rectangle, it's the default
        shape = AABB(ctx.self.get<Transform>(), halflen, offset);
    }

    return shape;
}

Shape getDefaultShape(const LoadContext& ctx) {
    // would be used for a default ellipse getter (if I need one):
    // const s32 radius = std::max(ctx.entityData.size.x, ctx.entityData.size.y) / 2;
    return AABB(ctx.self.get<Transform>(), ctx.entityData.size / 2, Vector2i());
}

Shape getDefaultShapeTile(const LoadContext& ctx) {
    const Transform& trans = ctx.self.get<Transform>();
    Vector2i halflen = ctx.entityData.size / 2;

    // tiles have discrete rotations of 0/90/180/270
    if (math::isBetween(trans.rotation, 89.0f, 91.0f) || math::isBetween(trans.rotation, 269.0f, 271.0f)) {
        halflen = Vector2i(halflen.y, halflen.x);
    }
    return AABB(ctx.parent.get<Transform>(), halflen, (trans._localPosition * trans.scale).as<s32>());
}

Shape readShapeOrDefault(const LoadContext& ctx, const char* key, Vector2i* dstOffset) {
    Shape shape;
    if (tryReadShape(ctx, key, &shape, dstOffset)) {
        return shape;
    }
    return ctx.parent.has<TileTag>() ? getDefaultShapeTile(ctx) : getDefaultShape(ctx);
}

template <>
bool tryRead(const JsonValue data, const char* key, s32* dst) {
    if (data.contains(key)) {
        *dst = data[key].getInt();
        return true;
    }
    return false;
}

template <>
bool tryRead(const JsonValue data, const char* key, f32* dst) {
    if (data.contains(key)) {
        *dst = data[key].getNumber();
        return true;
    }
    return false;
}

template <>
bool tryRead(const JsonValue data, const char* key, bool* dst) {
    if (data.contains(key)) {
        *dst = data[key].getBool();
        return true;
    }
    return false;
}

template <>
bool tryRead(const JsonValue data, const char* key, std::string* dst) {
    if (data.contains(key)) {
        *dst = data[key].getString();
        return true;
    }
    return false;
}

template <>
bool tryRead(const JsonValue data, const char* key, Color* dst) {
    if (data.contains(key)) {
        std::string hexString = readString(data, key);
        *dst = readColor(hexString);
        return true;
    }
    return false;
}

template <>
bool tryRead(const JsonValue data, const char* key, Depth* dst) {
    if (data.contains(key)) {
        std::string str = readString(data, key);
        *dst = readDepth(str);
        return true;
    }
    return false;
}

template <>
bool tryRead(const JsonValue data, const char* key, Vector2i* dst) {
    if (data.contains(key)) {
        tryRead(data[key], "x", &dst->x);
        tryRead(data[key], "y", &dst->y);
        return true;
    }
    return false;
}

template <>
bool tryRead(const JsonValue data, const char* key, Vector2f* dst) {
    if (data.contains(key)) {
        tryRead(data[key], "x", &dst->x);
        tryRead(data[key], "y", &dst->y);
        return true;
    }
    return false;
}

template <>
bool tryRead(const JsonValue data, const char* key, TileMapEntityDescriptor* dst) {
    if (data.contains(key)) {
        tryRead(data[key], "mapFile", &dst->mapFile);
        tryRead(data[key], "entityName", &dst->entityName);
        return true;
    }
    return false;
}

bool tryRead(const JsonValue data, const char* xKey, const char* yKey, Vector2f* dst) {
    bool foundOne = false;
    if (data.contains(xKey)) {
        dst->x = data[xKey].getNumber();
        foundOne = true;
    }
    if (data.contains(yKey)) {
        dst->y = data[yKey].getNumber();
        foundOne = true;
    }
    return foundOne;
}

bool tryRead(const JsonValue data, const char* xKey, const char* yKey, Vector2i* dst) {
    bool foundOne = false;
    if (data.contains(xKey)) {
        dst->x = data[xKey].getInt();
        foundOne = true;
    }
    if (data.contains(yKey)) {
        dst->y = data[yKey].getInt();
        foundOne = true;
    }
    return foundOne;
}

bool tryReadShape(const LoadContext& ctx, const char* key, Shape* dst, Vector2i* dstOffset) {
    if (ctx.values.contains(key)) {
        *dst = readShape(ctx, key, dstOffset);
        return true;
    }
    return false;
}

}  // namespace whal

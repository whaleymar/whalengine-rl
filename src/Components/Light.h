#pragma once

#include "Gfx/Color.h"
#include "Map/ComponentFactory.h"
#include "Physics/Shapes.h"
#include "Util/Types.h"
#include "Util/Vector.h"

namespace whal {

// this is implemented in a kind of jank way, but it's fast
struct PointLight : ISerialize<PointLight, ComponentFactory> {
    s32 radius = 1;
    s32 heightOffset = 0;
    Color color = Colors::White;
};

// slower than pointlight, but more control over shape
struct BoxLight : ISerialize<BoxLight, ComponentFactory> {
    s32 radius = 1;
    Vector2i offset;
    Color color = Colors::White;
    Vector2i halfLen;

    static void loadImpl(ecs::Entity entity, const LoadContext& ctx) {
        BoxLight light = entity.has<BoxLight>() ? entity.get<BoxLight>() : BoxLight{};

        tryRead(*ctx.values, "color", &light.color);
        tryRead(*ctx.values, "radius", &light.radius);
        light.halfLen = readShapeOrDefault(ctx, "Shape", &light.offset).getAABB().getHalf();
        entity.add(light);
    }
};

struct ShadowLight {
    s32 radius = 1;
    s32 heightOffset = 0;
    Color color = Colors::White;
};

}  // namespace whal

#pragma once

#include "Gfx/Color.h"
#include "Map/ComponentFactory.h"
#include "Map/Tiled.h"
#include "Util/Types.h"
#include "Util/Vector.h"

namespace whal {

// this is implemented in a kind of jank way, but it's fast
struct PointLight : ISerialize<PointLight, ComponentFactory> {
    s32 radius = 1;
    s32 heightOffset = 0;
    Color color = Colors::White;

    static void loadImpl(ecs::Entity entity, void* data) {
        const LoadContext& ctx = *static_cast<LoadContext*>(data);
        PointLight light = entity.has<PointLight>() ? entity.get<PointLight>() : PointLight{};
        if (!tryReadInt(ctx.values, "radiusTexels", &light.radius)) {
            // by default, use bigger dimension
            light.radius = std::max(ctx.entityData.size.x, ctx.entityData.size.y);
        }
        tryReadInt(ctx.values, "heightTexels", &light.heightOffset);
        tryReadColor(ctx.values, "Color", &light.color);
        entity.add(light);
    }
};

// slower than pointlight, but more control over shape
struct BoxLight {
    s32 radius = 1;
    s32 heightOffset = 0;
    Color color = Colors::White;
    Vector2i halfLen;
};

struct ShadowLight {
    s32 radius = 1;
    s32 heightOffset = 0;
    Color color = Colors::White;
};

}  // namespace whal

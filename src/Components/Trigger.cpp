#include "Trigger.h"

#include "Map/TiledParse.h"
#include "Util/JsonUtil.h"

namespace whal {

void Trigger::loadImpl(ecs::Entity entity, const LoadContext& ctx) {
    Trigger trigger = entity.has<Trigger>() ? entity.get<Trigger>() : Trigger{};

    std::string layerName;
    if (tryReadVal(*ctx.values, "Layer", &layerName)) {
        trigger.layerMask = CollisionLayer::fromString(layerName.c_str());
    }

    trigger.shape = readShapeOrDefault(ctx, "Shape", &trigger.offset);
    entity.add(trigger);
}

}  // namespace whal

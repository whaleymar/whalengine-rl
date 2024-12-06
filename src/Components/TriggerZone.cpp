#include "TriggerZone.h"

#include "Map/TiledParse.h"
#include "Util/JsonUtil.h"

namespace whal {

Trigger::Trigger(Shape shape_, CollisionLayer::Layer layer_, TriggerCallback callbackEnter, Vector2i offset_, TriggerCallback callbackExit,
                 TriggerCallback callbackStay)
    : shape(shape_), offset(offset_), layer(layer_), onTriggerEnter(callbackEnter), onTriggerExit(callbackExit), onTriggerStay(callbackStay) {}

void Trigger::loadImpl(ecs::Entity entity, void* data) {
    const LoadContext& ctx = *static_cast<LoadContext*>(data);
    Trigger trigger = entity.has<Trigger>() ? entity.get<Trigger>() : Trigger{};

    std::string layerName;
    if (tryReadVal(ctx.values, "Layer", &layerName)) {
        trigger.layer = CollisionLayer::fromString(layerName.c_str());
    }

    trigger.shape = readShapeOrDefault(ctx, entity, "Shape", &trigger.offset);
    entity.add(trigger);
}

}  // namespace whal

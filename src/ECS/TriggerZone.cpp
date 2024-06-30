#include "TriggerZone.h"

namespace whal {

Trigger::Trigger(Shape shape_, CollisionLayer::Layer layer_, TriggerCallback callbackEnter, Vector2i offset_, TriggerCallback callbackExit,
                 TriggerCallback callbackStay)
    : shape(shape_), offset(offset_), layer(layer_), onTriggerEnter(callbackEnter), onTriggerExit(callbackExit), onTriggerStay(callbackStay) {}

}  // namespace whal

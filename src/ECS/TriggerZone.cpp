#include "TriggerZone.h"

namespace whal {

Trigger::Trigger(Shape shape_, CollisionLayer::Layer layer_, TriggerCallback callbackEnter, TriggerCallback callbackExit,
                 TriggerCallback callbackStay)
    : shape(shape_), layer(layer_), onTriggerEnter(callbackEnter), onTriggerExit(callbackExit), onTriggerStay(callbackStay) {}

}  // namespace whal

#include "TriggerZone.h"

#include "whalECS/src/ECS.h"

namespace whal {

Trigger::Trigger(Shape shape_, TriggerCallback callbackEnter, TriggerCallback callbackExit, TriggerCallback callbackStay)
    : shape(shape_), onTriggerEnter(callbackEnter), onTriggerExit(callbackExit), onTriggerStay(callbackStay) {}

}  // namespace whal

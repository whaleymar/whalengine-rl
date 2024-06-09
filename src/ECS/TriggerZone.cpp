#include "TriggerZone.h"

namespace whal {

Trigger::Trigger(Shape shape_, TriggerCallback callbackEnter, TriggerCallback callbackExit, TriggerCallback callbackStay)
    : shape(shape_), onTriggerEnter(callbackEnter), onTriggerExit(callbackExit), onTriggerStay(callbackStay) {}

}  // namespace whal

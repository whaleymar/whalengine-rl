#include "TagSystems.h"

#include "Components/RailsControl.h"
#include "Components/Transform.h"

#include "Sys/System.h"

namespace whal {

void AudioListenerSystem::update() {
    if (getEntities().empty()) {
        return;
    }
    auto listenerEntity = first();
    Audio.setListenerPosition(listenerEntity.get<Transform>().positionPx);
}

}  // namespace whal

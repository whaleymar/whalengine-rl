#include "TagSystems.h"

#include "Components/RailsControl.h"
#include "Components/Transform.h"

#include "Sys/System.h"

namespace whal {

void AudioListenerSystem::update() {
    if (getEntitiesMutable().empty()) {
        return;
    }
    auto listenerEntity = first();
    Audio.setListenerPosition(listenerEntity.get<Transform2D>().position);
}

}  // namespace whal

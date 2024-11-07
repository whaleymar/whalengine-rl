#include "MouseCursorSystem.h"

#include "Components/Draw.h"

namespace whal {

void MouseCursorSystem::update() {
    for (auto [id, entity] : getEntitiesMutable()) {
        Vector2i position = System::input.getMouseWorld();

        // correct for cursor height
        const auto height = entity.get<Sprite>().frameSize.y;
        position -= Vector2i(0, height / 2);

        // update transform
        entity.get<Transform2D>().position = position;
    }
}

}  // namespace whal

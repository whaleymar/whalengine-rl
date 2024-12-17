#include "Tags.h"

#include "Components/Collision.h"
#include "Map/TiledParse.h"

namespace whal {

void TagLoader::loadImpl(ecs::Entity entity, void* data) {
    const LoadContext& ctx = *static_cast<LoadContext*>(data);

    bool hasTag = false;
    if (tryRead(ctx.values, "Player", &hasTag) && hasTag) {
        entity.add<Player>();
        hasTag = false;
    }

    if (tryRead(ctx.values, "Wiggle", &hasTag) && hasTag) {
        entity.add<Wiggle>();
        hasTag = false;
    }

    if (tryRead(ctx.values, "Invisible", &hasTag) && hasTag) {
        entity.add<Invisible>();
        hasTag = false;
    }

    if (tryRead(ctx.values, "BlocksLight", &hasTag) && hasTag) {
        entity.add<BlocksLight>();
        hasTag = false;
    }
}

}  // namespace whal

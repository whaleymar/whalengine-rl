#include "Tags.h"

#include "Components/Collider.h"
#include "Map/TiledParse.h"
#include "Sys/System.h"

namespace whal {

void TagLoader::loadImpl(ecs::Entity entity, const LoadContext& ctx) {
    bool hasTag = false;
    if (tryRead(*ctx.values, "Player", &hasTag) && hasTag) {
        entity.add<Player>();
        hasTag = false;
    }

    if (tryRead(*ctx.values, "Wiggle", &hasTag) && hasTag) {
        entity.add<Wiggle>();
        hasTag = false;
    }

    if (tryRead(*ctx.values, "Invisible", &hasTag) && hasTag) {
        entity.add<Invisible>();
        hasTag = false;
    }

    if (tryRead(*ctx.values, "BlocksLight", &hasTag) && hasTag) {
        entity.add<BlocksLight>();
        hasTag = false;
    }

    if (tryRead(*ctx.values, "Inactive", &hasTag) && hasTag) {
        // jank shit; i need children to have an independent activity flag!
        // TODO
        Schedule.flow({entity}).addWait(0.05).add([](ecs::Entity self) { self.deactivate(); }, entity);
        hasTag = false;
    }
}

}  // namespace whal

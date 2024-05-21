#include "ECS/Blaster.h"

#include "Game/Events.h"
#include "Systems/Event.h"
#include "Util/Vector.h"

#include "ECS/Entities/Projectile.h"
#include "ECS/Transform.h"
#include "ECS/Velocity.h"

void onBlasterFired(whal::Vector2i target) {
    using namespace whal;
    for (auto& [entityid, entity] : ProjectileSystem::getEntitiesRef()) {
        Transform trans = entity.get<Transform>();

        // change where the projectile starts (relative to shooting entity)
        Vector2i offset = {0, PIXELS_PER_TILE};
        Vector2i shotOrigin = trans.position + offset;
        Vector2f moveNormal = toFloatVec(target - shotOrigin).norm();

        Vector2f velocity;
        Blaster& blaster = entity.get<Blaster>();
        velocity = moveNormal * blaster.projectileSpeed;

        // auto totalVel = velocity + entity.get<Velocity>().total;
        auto totalVel = velocity;
        makeProjectile(shotOrigin, totalVel, blaster.projectileLifetimeSeconds, blaster.explosionRadius);

        // push shooter in opposite direction of projectile
        if (auto velOpt = entity.tryGet<Velocity>(); velOpt) {
            velOpt.value()->stable += moveNormal * -1 * blaster.shotKnockback;
        }

        // System::audio.play(Sfx::SHOTFIRED, 0.2); // TODO
    }
}

ProjectileSystem::ProjectileSystem() : mBlasterEventListener(whal::EventListener<whal::Vector2i>(&onBlasterFired)) {
    whal::System::eventMgr.registerListener(whal::Event::SHOOT_EVENT, mBlasterEventListener);
}

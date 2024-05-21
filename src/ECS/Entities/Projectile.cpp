#include "Projectile.h"

#include "Settings.h"

#include "ECS/AnimUtil.h"
#include "ECS/Animator.h"
#include "ECS/Collision.h"
#include "ECS/Draw.h"
#include "ECS/Entities/Explosion.h"
#include "ECS/Lib/ECS.h"
#include "ECS/Lifetime.h"
#include "ECS/Name.h"
#include "ECS/Radius.h"
#include "ECS/Transform.h"
#include "ECS/Velocity.h"
#include "Gfx/GfxUtil.h"
#include "Physics/IUseCollision.h"
#include "Util/MathUtil.h"

namespace whal {

void makeDefaultExplosion(ecs::Entity self) {
    // lifetime's onDeath callback
    f32 explosionRadius = self.get<Radius>().r;
    if (explosionRadius > 0) {
        Vector2i pos = self.get<Transform>().position;
        makeExplosionZone(pos, explosionRadius);
    }
}

void Explode(ecs::Entity self, ecs::Entity other, IUseCollision* selfCollider, IUseCollision* otherCollider, Vector2i moveNormal) {
    // make 2 knockback zones. One centered at the point of collision, and one with its far edge aligned with the collision edge
    // makeExplosionZone(selfCollider->getCollider().getPositionEdge(moveNormal) - moveNormal * PROJECTILE_EXPLOSION_RADIUS,
    //                   PROJECTILE_EXPLOSION_RADIUS);

    // nvm just make one big zone
    f32 explosionRadius = self.get<Radius>().r;
    if (explosionRadius > 0) {
        Vector2i pos = selfCollider->getCollider().getPositionEdge(moveNormal);
        makeExplosionZone(pos, explosionRadius);
    }
    self.kill();
}

void enableCollision(ecs::Entity entity) {
    entity.get<ActorCollider>().setIsCollidable(true);
}

Expected<ecs::Entity> makeProjectile(Vector2i position, Vector2f velocity, f32 lifetimeSeconds, f32 explosionRadius) {
    auto expected = ecs::ECS::getInstance().entity();
    if (!expected.isExpected()) {
        return expected.error();
    }

    auto entity = expected.value();

    // the sprite is pointing down by default. Get angle between for rotation
    Vector2f moveNormal = velocity.norm();
    // Vector2f moveNormal = Vector2f::unitDown;
    Vector2f referenceAngle = Vector2f::unitDown;
    f32 dot = moveNormal.dot(referenceAngle);
    f32 det = moveNormal.det(referenceAngle);
    f32 angleRadians = std::atan2(det, dot);
    Transform trans(position, angleRadians * RAD_TO_DEG);

    Velocity vel(velocity);
    s32 len = 4;
    s32 halflenPixels = len / 2 * PIXELS_PER_TEXEL;

    entity.add(trans);
    entity.add(vel);
    entity.add(Name("PROJECTILE"));
    entity.add(Lifetime(lifetimeSeconds, &makeDefaultExplosion));
    entity.add(Radius(explosionRadius));

    static const AnimInfo animInfo = {{"effect/bluefire", 0, 4, 0.1}};
    Animator animator;
    loadAnimations(animator, animInfo);
    entity.add(animator);
    Sprite sprite = Sprite(Depth::Player, animator.getFrame());
    sprite.scale = {0.5, 0.5};
    entity.add(sprite);

    // add collider slightly after creation so it doesn't collide with shooter
    // TODO this still sucks, should use a layer mask or something so it can't collide with shooter
    auto collider = ActorCollider(trans, {halflenPixels, halflenPixels}, Material::None, &Explode, true);
    collider.setIsCollidable(false);
    entity.add<ActorCollider>(collider);
    System::schedule.after(&enableCollision, 0.075, entity);

    return entity;
}

}  // namespace whal

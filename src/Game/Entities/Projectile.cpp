#include "Projectile.h"

#include "Physics/Collision/Shapes.h"
#include "Settings.h"
#include "whalECS/src/ECS.h"

#include "ECS/AnimUtil.h"
#include "ECS/Animator.h"
#include "ECS/Collision.h"
#include "ECS/Draw.h"
#include "ECS/Lifetime.h"
#include "ECS/Name.h"
#include "ECS/Transform.h"
#include "ECS/Velocity.h"
#include "Explosion.h"

#include "Physics/IUseCollision.h"
#include "Systems/System.h"
#include "Util/MathUtil.h"

using namespace whal;

void makeDefaultExplosion(ecs::Entity self) {
    // lifetime's onDeath callback
    f32 explosionRadius = self.get<Circle>().getRadius();
    if (explosionRadius > 0) {
        Vector2i pos = self.get<Transform2D>().position;
        makeExplosionZone(pos, explosionRadius);
    }
}

void Explode(ecs::Entity self, ecs::Entity other, IUseCollision* selfCollider, IUseCollision* otherCollider, Vector2i moveNormal) {
    f32 explosionRadius = self.get<Circle>().getRadius();
    if (explosionRadius > 0) {
        Vector2i pos = selfCollider->getCollider().getPositionEdge(moveNormal);
        makeExplosionZone(pos, explosionRadius);
    }
    self.kill();
}

Expected<ecs::Entity> makeProjectile(Vector2i position, Vector2f velocity, f32 lifetimeSeconds, f32 explosionRadius) {
    auto expected = System::ecs->entity(false);
    if (!expected.isExpected()) {
        return expected.error();
    }

    auto entity = expected.value();
    auto _ = ecs::DeferActivate(entity);

    // the sprite is pointing down by default. Get angle between for rotation
    Vector2f moveNormal = velocity.norm();
    // Vector2f moveNormal = Vector2f::unitDown;
    Vector2f referenceAngle = Vector2f::unitDown;
    f32 dot = moveNormal.dot(referenceAngle);
    f32 det = moveNormal.det(referenceAngle);
    f32 angleRadians = std::atan2(det, dot);
    Transform2D trans(position, angleRadians * RAD_TO_DEG);

    Velocity vel(velocity);
    s32 len = 4;
    s32 halflenPixels = len / 2 * PIXELS_PER_TEXEL;

    entity.add(trans);
    entity.add(vel);
    entity.add(Name("PROJECTILE"));
    entity.add(Lifetime(lifetimeSeconds, &makeDefaultExplosion));
    entity.add(Circle(Vector2i(), explosionRadius));

    static const AnimInfo animInfo = {{"effect/bluefire", 0, 4, 0.1}};
    Animator animator;
    loadAnimations(animator, animInfo);
    entity.add(animator);
    Sprite sprite = Sprite(Depth::Player, animator.getFrame());
    sprite.scale = {0.5, 0.5};
    entity.add(sprite);

    // add collider slightly after creation so it doesn't collide with shooter
    // TODO this still sucks, should use a layer mask or something so it can't collide with shooter
    auto collider = ActorCollider(trans, {halflenPixels, halflenPixels}, WorldMaterial::None, &Explode, true);
    collider.setIsCollidable(false);
    entity.add<ActorCollider>(collider);

    auto enableCollision = [](ecs::Entity entity) -> void { entity.get<ActorCollider>().setIsCollidable(true); };
    System::schedule.after(enableCollision, 0.075, entity);

    return entity;
}

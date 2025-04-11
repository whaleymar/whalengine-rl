#include "ComponentFactory.h"

#include "Components/Animator.h"
#include "Components/Collider.h"
#include "Components/Draw.h"
#include "Components/Lifetime.h"
#include "Components/Light.h"
#include "Components/MonoBehavior.h"
#include "Components/ParticleEmitter.h"
#include "Components/PlayerControl.h"
#include "Components/RailsControl.h"
#include "Components/RigidBody.h"
#include "Components/Tags.h"
#include "Components/Transform.h"
#include "Components/Trigger.h"
#include "Components/Velocity.h"
#include "Map/AnimationFactory.h"
#include "Map/Tiled.h"
#include "Map/TiledParse.h"
#include "Serializer.h"
#include "Sys/System.h"
#include "Util/JsonUtil.h"
#include "Util/SerializeUtil.h"

namespace whal {

// For a lot of components, I *don't* want to serialize the whole thing.
// - E.g. Sprite/Animator I would just want a string or two.
// - I can use non-default serde implementations for those
void ComponentFactory::initEcsSerializer() {
    addDefault<Transform>();
    // addDefault<Sprite>(); // has shader pointer (i would want to store the string name)
    addDefault<DrawRect>();
    addDefault<DrawStraightLine>();
    addDefault<TextSprite>();
    addDefault<DrawBezierQuad>();
    addDefault<SpriteOutline>();
    // addDefault<Animator>(); // complex class (i wouldn't want to default-serialize this anyway, probably just the string name + brain callback
    // name?)
    addDefault<Velocity>();
    addDefault<PlayerControl>();
    // addDefault<Collider>(); // complex class
    // addDefault<Wiggle>(); // has callback
    addDefault<PointLight>();
    addDefault<BoxLight>();
    addDefault<ShadowLight>();
    // addDefault<Lifetime>(); // has callback
    addDefault<ParticleEmitter>();
    addDefault<RigidBody>();
    // addDefault<TileMapLayer>(); // don't want default serializer
    // addDefault<Trigger>(); // has callbacks
    // addDefault<MonoBehavior>();
}

namespace stl {
template <class ForwardIt, class T = typename std::iterator_traits<ForwardIt>::value_type>
static void replace(ForwardIt first, ForwardIt last, const T& old_value, const T& new_value) {
    for (; first != last; ++first)
        if (*first == old_value)
            *first = new_value;
}
}  // namespace stl

const TiledDeserialize* ComponentFactory::get(const char* componentName) {
    ecs::Entity cmp = World.lookup(componentName);
    if (!cmp.isValid()) {
        // Component with that name not found
        return nullptr;
    }

    return cmp.tryGet<TiledDeserialize>();
}

void ComponentFactory::init() {
    initEcsSerializer();
    initTiledLoader();
}

static bool loadCheckpoints(const nlohmann::json& checkpointData, std::vector<RailsControl::CheckPoint>& dstCheckpoints, const LoadContext& ctx);

void ComponentFactory::initTiledLoader() {
    addDefaultTiledLoader<SpriteOutline>();
    addDefaultTiledLoader<PointLight>();
    addDefaultTiledLoader<Lifetime>();
    addDefaultTiledLoader<PlayerControl>();
    addDefaultTiledLoader<Jumper>();
    addDefaultTiledLoader<RigidBody>();

    // Sprite
    TILED_LOADER(
        Sprite, Sprite sprite = entity.has<Sprite>() ? entity.get<Sprite>() : Sprite{}; tryRead(*ctx.values, "Color", &sprite.color);

        f32 brightness; if (tryRead(*ctx.values, "Brightness", &brightness)) { sprite.color.scale(brightness); }

                        std::string spritePath = "";
        if (tryRead(*ctx.values, "Sprite", &spritePath)) { stl::replace(spritePath.begin(), spritePath.end(), '\\', '/'); } auto eSprite =
            Sprite::fromPath(spritePath.c_str());
        if (eSprite.isExpected()) {
            sprite.frameSize = eSprite.value().frameSize;
            sprite.atlasPosition = eSprite.value().atlasPosition;
            entity.add(sprite);
        } else { print("Error: Coudn't find frame for sprite:", spritePath); }

    );

    // DrawRect
    TILED_LOADER(
        DrawRect, DrawRect draw = entity.has<DrawRect>() ? entity.get<DrawRect>() : DrawRect{}; draw.frameSize = ctx.entityData.size;

        tryRead(*ctx.values, "Color", &draw.color);

        f32 brightness; if (tryRead(*ctx.values, "Brightness", &brightness)) { draw.color.scale(brightness); } entity.add(draw);

    );

    // DrawText
    TILED_LOADER(TextSprite, TextSprite text = entity.has<TextSprite>() ? entity.get<TextSprite>() : TextSprite{};

                 tryRead(*ctx.values, "color", &text.color); tryRead(*ctx.values, "text", &text.text);
                 tryRead(*ctx.values, "center", &text.isCentered); text.frameSize = ctx.entityData.size; entity.add(text);

    );

    // Trigger
    TILED_LOADER(
        Trigger, Trigger trigger = entity.has<Trigger>() ? entity.get<Trigger>() : Trigger{};

        std::string layerName;
        if (tryReadVal(*ctx.values, "Layer", &layerName)) { trigger.layerMask = CollisionLayer::fromString(layerName.c_str()); }

        trigger.shape = readShapeOrDefault(ctx, "Shape", &trigger.offset);
        entity.add(trigger);

    );

    // RailsControl
    TILED_LOADER(
        RailsControl, std::vector<RailsControl::CheckPoint> checkpoints; bool isCycle = false;
        if (ctx.values->contains("Checkpoints")) {
            s32 id = (*ctx.values)["Checkpoints"];
            const nlohmann::json checkPointObj = ctx.allObjects.at(ctx.idToIndex.at(id).first);
            isCycle = loadCheckpoints(checkPointObj, checkpoints, ctx);
        }

        RailsControl rails = entity.has<RailsControl>() ? entity.get<RailsControl>() : RailsControl{};
        rails.setCheckpoints(checkpoints, entity.get<Transform>(), entity);

        std::string cycleBehavior = "ManualStart"; tryRead(*ctx.values, "CycleBehavior", &cycleBehavior); if (isCycle) {
            if (cycleBehavior == "Automatic") {
                rails.endBehavior = RailsControl::CycleBehavior::AUTOMATIC_LOOP;
            } else if (cycleBehavior == "ManualStart") {
                rails.endBehavior = RailsControl::CycleBehavior::MANUAL_FIRSTSTEP_LOOP;
            } else {
                rails.endBehavior = RailsControl::CycleBehavior::MANUAL_ALLSTEPS_LOOP;
            }
        } else {
            if (cycleBehavior == "Automatic") {
                rails.endBehavior = RailsControl::CycleBehavior::AUTOMATIC_BACKTRACK;
            } else if (cycleBehavior == "ManualStart") {
                rails.endBehavior = RailsControl::CycleBehavior::MANUAL_FIRSTSTEP_BACKTRACK;
            } else {
                rails.endBehavior = RailsControl::CycleBehavior::MANUAL_ALLSTEPS_BACKTRACK;
            }
        }

        tryRead(*ctx.values, "speed", &rails.speed);
        tryRead(*ctx.values, "waitTime", &rails.waitTime);

        entity.add(rails);

    );

    // ParticleEmitter
    TILED_LOADER(ParticleEmitter, ParticleEmitter emitter = entity.has<ParticleEmitter>() ? entity.get<ParticleEmitter>() : ParticleEmitter{};

                 tryRead(*ctx.values, "maxSpeed", &emitter.maxSpeed); tryRead(*ctx.values, "particlesPerSecond", &emitter.particlesPerSecond);
                 tryReadVal(*ctx.values, "Direction", &emitter.direction); tryReadVal(*ctx.values, "Material", &emitter.material);
                 tryRead(*ctx.values, "Depth", &emitter.depth); tryRead(*ctx.values, "LifetimeMultiplier", &emitter.lifetimeMultiplier);

                 Shape emitterShape = readShapeOrDefault(ctx, "Shape", &emitter.offset); emitter.aabbHalf = emitterShape.getAABB().getHalf();
                 entity.add(emitter);

    );

    // Collider
    TILED_LOADER(
        Collider, Collider collider = entity.has<Collider>() ? entity.get<Collider>() : Collider{};
        CollisionDir collisionDir = collider.getCollisionDir(); WorldMaterial material = collider.getMaterial();

        if (tryReadVal(*ctx.values, "CollisionDir", &collisionDir)) {
            collider.setCollisionDir(collisionDir);
        } if (tryReadVal(*ctx.values, "Material", &material)) { collider.setMaterial(material); } tryReadVal(*ctx.values, "Type",
                                                                                                             &collider.mPhysicsBody);

        std::string layerName; if (tryReadVal(*ctx.values, "Layer", &layerName)) {
            // can have multiple values. Written as "layername,layername,layername"
            u16 mask = 0;
            size_t curIx = 0;
            while (true) {
                size_t commaIx = layerName.find(",", curIx);
                if (commaIx == std::string::npos) {
                    mask |= CollisionLayer::fromString(layerName.substr(curIx).c_str());
                    break;
                }
                // add layer and update curIx
                mask |= CollisionLayer::fromString(layerName.substr(curIx, commaIx - curIx).c_str());
                curIx = commaIx + 1;
            }
            // collider.setCollisionMask(CollisionLayer::fromString(layerName.c_str()));
            collider.setCollisionMask(mask);
        }

                               collider.setShape(readShapeOrDefault(ctx, "Shape", &collider.mOffset).getAABB());
        entity.add(collider);

    );

    // Animator
    TILED_LOADER(
        Animator, Sprite sprite = entity.has<Sprite>() ? entity.get<Sprite>() : Sprite{};
        std::string animatorName = readString(*ctx.values, "Animator");
        Animator animator = Animator::fromAnimation(AnimationFactory::get(animatorName.c_str())); entity.add(animator);
        sprite.setFrame(animator.getFrame());

        s32 rotation; if (tryRead(*ctx.values, "rotationDegrees", &rotation)) { entity.get<Transform>().rotation = rotation; }

        tryRead(*ctx.values, "Color", &sprite.color);

        f32 brightness; if (tryRead(*ctx.values, "Brightness", &brightness)) { sprite.color.scale(brightness); } entity.add(sprite);

    );

    // BoxLight
    TILED_LOADER(BoxLight, BoxLight light = entity.has<BoxLight>() ? entity.get<BoxLight>() : BoxLight{};

                 tryRead(*ctx.values, "color", &light.color); tryRead(*ctx.values, "radius", &light.radius);
                 light.halfLen = readShapeOrDefault(ctx, "Shape", &light.offset).getAABB().getHalf(); entity.add(light);

    );

    // Velocity
    TILED_LOADER(Velocity, Velocity velocity = entity.has<Velocity>() ? entity.get<Velocity>() : Velocity{};
                 tryRead(*ctx.values, "stable", &velocity.stable);

                 entity.add(velocity);

    );

    TILED_LOADER(
        TagLoader, bool hasTag = false; if (tryRead(*ctx.values, "Player", &hasTag) && hasTag) {
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

    );
}

// returns true if checkpoints form a cycle
static bool loadCheckpoints(const nlohmann::json& checkpointData, std::vector<RailsControl::CheckPoint>& dstCheckpoints, const LoadContext& ctx) {
    static const char* KEY_VALUE = "value";

    // generic rewrite:
    const s32 parentX = readInt(checkpointData, "x");
    const s32 parentY = readInt(checkpointData, "y");
    assert(checkpointData.contains("properties") && "Checkpoint object has no properties");
    const auto& properties = checkpointData["properties"];
    std::vector<Ease> moveProps;
    for (const auto& moveProperty : properties) {
        const s32 moveIx = moveProperty[KEY_VALUE];
        moveProps.push_back(static_cast<Ease>(moveIx));
    }

    bool isCycle = checkpointData.contains("polygon");
    std::string pathKey;
    if (isCycle) {
        pathKey = "polygon";
    } else {
        pathKey = "polyline";
    }
    size_t ix = 0;
    for (const auto& point : checkpointData[pathKey]) {
        const s32 x = readInt(point, "x");
        const s32 y = readInt(point, "y");
        const Vector2i mapPos = {x + parentX, parentY + y};
        const Vector2i trans = getMapTransform(mapPos, Vector2i::ZERO, ctx.parent).positionPx;

        Ease moveType;
        if (ix >= moveProps.size()) {
            print("Checkpoints object with ID", readInt(checkpointData, "id"), "has", moveProps.size(), "move type params but it has more points");
            moveType = Ease::Linear;
        } else {
            moveType = moveProps[ix];
        }
        RailsControl::CheckPoint chkPoint(trans, moveType);
        dstCheckpoints.push_back(chkPoint);

        ix++;
    }

    return isCycle;
}

}  // namespace whal

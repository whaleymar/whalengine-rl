#include "SpriteRenderSystem.h"

#include "Common.h"
#include "Components/Draw.h"
#include "Components/Font.h"
#include "Components/Tags.h"
#include "Components/Transform.h"
#include "ECS.h"
#include "Gfx/Coordinates.h"
#include "Gfx/RaylibUtil.h"
#include "Gfx/Shader.h"
#include "Gfx/ShaderManager.h"
#include "Gfx/Texture.h"
#include "Physics/OBB.h"
#include "Settings.h"
#include "Sys/System.h"
#include "Sys/Time.h"

namespace whal {

using namespace rutil;

static bool isChildDrawable(ecs::Entity child, const ecs::SystemBase* system);
static void tryDrawBehind(ecs::Entity child, const gfx::RenderContext& ctx, const ecs::SystemBase* system);
static void tryDrawOnTop(ecs::Entity child, const gfx::RenderContext& ctx, const ecs::SystemBase* system);
static void drawEntityAndChildren(ecs::Entity e, const gfx::RenderContext& ctx, const ecs::SystemBase* system);

// draw functions for Drawable engine components:
static void drawSprite(ecs::Entity entity, const gfx::RenderContext& ctx);
static void queueSprite(ecs::Entity entity, gfx::RenderQueue& queue);
static void drawTextSprite(ecs::Entity entity, const gfx::RenderContext& ctx);
static void queueTextSprite(ecs::Entity entity, gfx::RenderQueue& queue);
static void drawRectangle(ecs::Entity entity, const gfx::RenderContext& ctx);
static void queueRectangle(ecs::Entity entity, gfx::RenderQueue& queue);
static void drawLine(ecs::Entity entity, const gfx::RenderContext& ctx);
static void queueLine(ecs::Entity entity, gfx::RenderQueue& queue);
static void drawBezier(ecs::Entity entity, const gfx::RenderContext& ctx);
static void queueBezier(ecs::Entity entity, gfx::RenderQueue& queue);

// misc utility :
struct LinePoints {
    Vector2f p1;
    Vector2f p2;
};

static LinePoints getRotatedPoints(Vector2f position, Transform trans, DrawStraightLine line);

static const Shader* S_CURRENT_SHADER;
static const Shader* S_DEFAULT_SHADER = nullptr;  // a pointer to the default shader, though I use nullptr as a sentinel mostly

SpriteRenderSystem::SpriteRenderSystem() {
    // register drawable engine components

    World.component<Sprite>().add<IDrawable>({
        .draw = drawSprite,
        .queue = queueSprite,
    });
    World.component<TextSprite>().add<IDrawable>({
        .draw = drawTextSprite,
        .queue = queueTextSprite,
    });
    World.component<DrawRect>().add<IDrawable>({
        .draw = drawRectangle,
        .queue = queueRectangle,
    });
    World.component<DrawStraightLine>().add<IDrawable>({
        .draw = drawLine,
        .queue = queueLine,
    });
    World.component<DrawBezierQuad>().add<IDrawable>({
        .draw = drawBezier,
        .queue = queueBezier,
    });

    // init shader pointers
    S_DEFAULT_SHADER = &ShaderMgr::get("DefaultSprite");
}

void SpriteRenderSystem::draw(const gfx::EntityRenderInfo& eCtx, const gfx::RenderContext& ctx) const {
    // this shader was set for us by the Renderer
    S_CURRENT_SHADER = eCtx.shader == S_DEFAULT_SHADER ? nullptr : eCtx.shader;
    drawEntityAndChildren(eCtx.entity, ctx, this);
}

void SpriteRenderSystem::addToQueue(gfx::RenderQueue& queue) const {
    for (const auto [entityid, entity] : getEntities()) {
        if (!entity.has<Invisible>()) {
            entity.getTrait<IDrawable>().queue(entity, queue);
        }
    }
}

void drawSprite(ecs::Entity child, const gfx::RenderContext& ctx) {
    const Transform& transform = child.get<Transform>();

    const Sprite& sprite = child.get<Sprite>();
    const rl::Rectangle srcRect = rl::Rectangle{sprite.atlasPosition.x, sprite.atlasPosition.y, math::sign(transform.scale.x) * sprite.frameSize.x,
                                                math::sign(transform.scale.y) * sprite.frameSize.y};
    const gfx::RaylibDrawParams params = gfx::getDrawParams(transform, sprite.frameSize);

    trySetShader(sprite.shader);
    bool isUI = transform.depth == Depth::Debug || transform.depth == Depth::UIFar || transform.depth == Depth::UIClose;
    gfx::DrawSpriteHDR(ctx.atlas.getTexture(), srcRect, params.rect, params.origin, transform.rotation, sprite.color.asRL(),
                       gfx::DrawMetaData{.depth = static_cast<u8>(transform.depth), .isUI = isUI}.asRL(sprite, ctx.atlas.getSize()), sprite.custom0b,
                       sprite.custom0a, sprite.isFlagSet(Sprite::PassSpriteCenter));
}

void queueSprite(ecs::Entity entity, gfx::RenderQueue& queue) {
    const Sprite& sprite = entity.get<Sprite>();
    const Transform& trans = entity.get<Transform>();
    AABB bb = trans.rotation == 0.0f ? AABB(trans, sprite.frameSize.as<s32>() / 2, Vector2i()) :
                                       OBB(trans.getRotatedPosition().round(), sprite.frameSize.as<s32>() / 2, trans.rotation).getBoundingAABB();

    // float height should affect bounding box (for culling) but not Y sorting
    s32 floatOffset = static_cast<s32>(trans.z * FLOAT_HEIGHT_MULT);
    bb.getPositionMut().y += floatOffset;
    queue.add(gfx::EntityPreRenderInfo{
        .boundingBox = bb,
        .transform = &trans,
        .ysortPosition = bb.bottom() - floatOffset,
        .entity = entity,
        .shader = sprite.shader,
    });
}

void drawRectangle(ecs::Entity entity, const gfx::RenderContext& ctx) {
    const DrawRect& rect = entity.get<DrawRect>();
    const Transform& trans = entity.get<Transform>();

    const auto frameSize = rect.frameSize.as<f32>();
    const gfx::RaylibDrawParams params = gfx::getDrawParams(trans, frameSize);

    trySetDefaultShader();
    bool isUI = trans.depth == Depth::Debug || trans.depth == Depth::UIFar || trans.depth == Depth::UIClose;
    gfx::DrawRectangleHDR(params.rect, params.origin, trans.rotation, rect.color,
                          gfx::DrawMetaData{.depth = static_cast<u8>(trans.depth), .isUI = isUI});
}

void queueRectangle(ecs::Entity entity, gfx::RenderQueue& queue) {
    const DrawRect& draw = entity.get<DrawRect>();
    const Transform& trans = entity.get<Transform>();
    AABB bb = trans.rotation == 0.0f ? AABB(trans, draw.frameSize / 2) :
                                       OBB(trans.getRotatedPosition().round(), draw.frameSize / 2, trans.rotation).getBoundingAABB();

    // float height should affect bounding box (for culling) but not Y sorting
    s32 floatOffset = static_cast<s32>(trans.z * FLOAT_HEIGHT_MULT);
    bb.getPositionMut().y += floatOffset;
    queue.add(gfx::EntityPreRenderInfo{
        .boundingBox = bb,
        .transform = &trans,
        .ysortPosition = bb.bottom() - floatOffset,
        .entity = entity,
    });
}

void drawBezier(ecs::Entity entity, const gfx::RenderContext& ctx) {
    // TODO this is not working idk why
    const Transform& trans = entity.get<Transform>();
    const DrawBezierQuad& bezier = entity.get<DrawBezierQuad>();
    rl::Vector2 p1 = trans.getRotatedPosition().asRL();
    rl::Vector2 controlPoint = trans.apply(bezier.controlPointOffset.as<f32>()).asRL();
    rl::Vector2 p2 = trans.apply(bezier.endPointOffset.as<f32>()).asRL();

    trySetDefaultShader();
    bool isUI = trans.depth == Depth::Debug || trans.depth == Depth::UIFar || trans.depth == Depth::UIClose;
    gfx::DrawSplineSegmentBezierQuadraticHDR(p1, controlPoint, p2, bezier.thickness * VIRTUAL_SCREEN_RATIO, bezier.color,
                                             gfx::DrawMetaData{.depth = static_cast<u8>(trans.depth), .isUI = isUI});
}

void queueBezier(ecs::Entity entity, gfx::RenderQueue& queue) {
    const DrawBezierQuad& line = entity.get<DrawBezierQuad>();
    const Transform& trans = entity.get<Transform>();
    AABB bb = AABB::fromPoints(trans.positionPx, trans.positionPx + line.controlPointOffset, trans.positionPx + line.endPointOffset);

    // float height should affect bounding box (for culling) but not Y sorting
    s32 floatOffset = static_cast<s32>(trans.z * FLOAT_HEIGHT_MULT);
    bb.getPositionMut().y += floatOffset;
    queue.add(gfx::EntityPreRenderInfo{
        .boundingBox = bb,
        .transform = &trans,
        .ysortPosition = bb.bottom() - floatOffset,
        .entity = entity,
    });
}

void drawLine(ecs::Entity entity, const gfx::RenderContext& ctx) {
    const auto line = entity.get<DrawStraightLine>();
    const Transform& trans = entity.get<Transform>();
    const LinePoints points = getRotatedPoints(trans.position, entity.get<Transform>(), line);
    const Vector2f p1 = worldToRenderCoords(points.p1);
    const Vector2f p2 = worldToRenderCoords(points.p2);

    const f32 len = (p2 - p1).len();
    if (math::isNearZero(len, 0.01)) {
        return;
    }

    trySetDefaultShader();
    bool isUI = trans.depth == Depth::Debug || trans.depth == Depth::UIFar || trans.depth == Depth::UIClose;
    auto const colorBuf = gfx::DrawMetaData{.depth = static_cast<u8>(trans.depth), .isUI = isUI};
    if (line.segmentLength == 0 || line.segmentGapLength == 0) {
        // draw as single segment
        gfx::DrawLineHDR(p1.asRL(), p2.asRL(), line.thickness * VIRTUAL_SCREEN_RATIO, line.color, colorBuf);
        return;
    }

    const Vector2f norm = (p2 - p1) / len;
    const Vector2f segmentStep = norm * static_cast<f32>(line.segmentLength) * VIRTUAL_SCREEN_RATIO;
    const Vector2f gapStep = norm * static_cast<f32>(line.segmentGapLength) * VIRTUAL_SCREEN_RATIO;
    Vector2f current = p1;
    if (line.segmentCycleTime > 0.0) {
        f32 offsetT = std::fmod(Time.getElapsed(), line.segmentCycleTime) / line.segmentCycleTime;
        current += (segmentStep + gapStep) * offsetT;
    }

    while (true) {
        // check if we're past p2
        if (math::sign(p2.x - p1.x) != math::sign(p2.x - current.x) || math::sign(p2.y - p1.y) != math::sign(p2.y - current.y)) {
            return;
        }

        Vector2f next = current + segmentStep;
        if (math::sign(p2.x - p1.x) != math::sign(p2.x - next.x) || math::sign(p2.y - p1.y) != math::sign(p2.y - next.y)) {
            // don't draw beyond p2
            gfx::DrawLineHDR(current.asRL(), p2.asRL(), line.thickness * VIRTUAL_SCREEN_RATIO, line.color, colorBuf);
            return;
        }

        gfx::DrawLineHDR(current.asRL(), next.asRL(), line.thickness * VIRTUAL_SCREEN_RATIO, line.color, colorBuf);
        current += segmentStep + gapStep;
    }
}

void queueLine(ecs::Entity entity, gfx::RenderQueue& queue) {
    const DrawStraightLine& line = entity.get<DrawStraightLine>();
    const Transform& trans = entity.get<Transform>();
    const LinePoints points = getRotatedPoints(trans.position, trans, line);
    AABB bb = AABB::fromPoints(points.p1.as<s32>(), points.p2.as<s32>());

    // float height should affect bounding box (for culling) but not Y sorting
    s32 floatOffset = static_cast<s32>(trans.z * FLOAT_HEIGHT_MULT);
    bb.getPositionMut().y += floatOffset;
    queue.add(gfx::EntityPreRenderInfo{
        .boundingBox = bb,
        .transform = &trans,
        .ysortPosition = bb.bottom() - floatOffset,
        .entity = entity,
    });
}

void drawTextSprite(ecs::Entity entity, const gfx::RenderContext& ctx) {
    constexpr s32 spacing = 0;         // PARAM
    const Color tint = Colors::White;  // PARAM

    const TextSprite text = entity.get<TextSprite>();
    const Transform& trans = entity.get<Transform>();

    // rotation pivot correction
    Vector2f pivotOffsetScreen = trans.pivotOffset.as<f32>() * Vector2f(1, -1) * VIRTUAL_SCREEN_RATIO;
    Vector2f frameSize = text.frameSize.as<f32>();

    // scale to full resolution
    frameSize = (frameSize * VIRTUAL_SCREEN_RATIO * trans.scale).absolute();
    Vector2f screenPosition = trans.position * Vector2f(VIRTUAL_SCREEN_RATIO, -VIRTUAL_SCREEN_RATIO);

    // UNUSED
    // {
    //  drawing one line:
    //  Vector2 textDimensions = MeasureTextEx(DEFAULT_FONT, draw.text, FONT_SIZE, spacing);
    //  dstPosition -= Vector2f(textDimensions.x / 2, textDimensions.y);
    //  DrawTextEx(DEFAULT_FONT, draw.text, Vector2(dstPosition.x, dstPosition.y), FONT_SIZE, spacing, ColorTint(draw.color, tint));
    // }

    // TODO needs adjustment based on rotation? Currently not working for multi-line text
    // it's definitely because my DrawTextBoxed function doesn't calculate Y values correctly
    // drawing wrapped:
    // dstPosition -= frameSize * Vector2f(0.5, 0.5);  // original
    screenPosition -= frameSize * Vector2f(0.5, 0.0);  // trying something new
    // needs half tile offset for some reason; might be an issue with map data:
    // dstPosition += Vector2f(0, FPIXELS_PER_TILE / 2 * VIRTUAL_SCREEN_RATIO);

    rl::Rectangle dstRect = rl::Rectangle(screenPosition.x, screenPosition.y, frameSize.x, frameSize.y);

    gfx::RaylibDrawParams params = gfx::RaylibDrawParams{
        .rect = dstRect,
        .origin = rl::Vector2{0, 0},
    };

    trySetDefaultShader();
    bool isUI = trans.depth == Depth::Debug || trans.depth == Depth::UIFar || trans.depth == Depth::UIClose;
    auto const colorBuf = gfx::DrawMetaData{.depth = static_cast<u8>(trans.depth), .isUI = isUI};
    gfx::DrawTextBoxed(World.get<Font>().normal, text.text.c_str(), params, getFontSize(), spacing, text.isWrapped, text.isCentered,
                       text.color * tint, trans.rotation, pivotOffsetScreen, colorBuf, trans.scale.absolute().asRL());
}

void queueTextSprite(ecs::Entity entity, gfx::RenderQueue& queue) {
    const TextSprite& draw = entity.get<TextSprite>();
    const Transform& trans = entity.get<Transform>();

    AABB bb = trans.rotation == 0.0f ? AABB(trans, draw.frameSize / 2) :
                                       OBB(trans.getRotatedPosition().round(), draw.frameSize / 2, trans.rotation).getBoundingAABB();

    // float height should affect bounding box (for culling) but not Y sorting
    s32 floatOffset = static_cast<s32>(trans.z * FLOAT_HEIGHT_MULT);
    bb.getPositionMut().y += floatOffset;
    queue.add(gfx::EntityPreRenderInfo{
        .boundingBox = bb,
        .transform = &trans,
        .ysortPosition = bb.bottom() - floatOffset,
        .entity = entity,
    });
}

// TODO do I need to call invisible here? isMatch should cover that
bool isChildDrawable(ecs::Entity child, const ecs::SystemBase* system) {
    return !SpriteRenderSystem::getEntities().contains(child.id()) && !child.has<Invisible>() && !child.has<ecs::OverrideAttributeIgnoreChildren>() &&
           system->isMatch(child);
}

void tryDrawBehind(ecs::Entity child, const gfx::RenderContext& ctx, const ecs::SystemBase* system) {
    if (!child.has<DrawOnTopOfParent>() && isChildDrawable(child, system)) {
        drawEntityAndChildren(child, ctx, system);
    }
}

void tryDrawOnTop(ecs::Entity child, const gfx::RenderContext& ctx, const ecs::SystemBase* system) {
    if (child.has<DrawOnTopOfParent>() && isChildDrawable(child, system)) {
        drawEntityAndChildren(child, ctx, system);
    }
}

void drawEntityAndChildren(ecs::Entity e, const gfx::RenderContext& ctx, const ecs::SystemBase* system) {
    e.forChild(tryDrawBehind, false, ctx, system);
    // e.getTrait<IDrawable>().draw(e, ctx);
    // if I want to draw all matching components:
    e.forTrait<IDrawable>([](ecs::Entity self, ecs::Entity cmp, const gfx::RenderContext& ctx) { cmp.get<IDrawable>().draw(self, ctx); }, ctx);
    // (i don't think I want that because draw order is arbitrarily based on component registration order -- easier to make a child component)
    e.forChild(tryDrawOnTop, false, ctx, system);
}

LinePoints getRotatedPoints(Vector2f position, Transform trans, DrawStraightLine line) {
    Vector2f startPos;
    Vector2f endPos;
    if (line.isRotateAboutCenter) {
        Vector2f halfLine = Vector2f::fromAngle(trans.rotation) * static_cast<f32>(line.length) * 0.5f;
        startPos = position - halfLine;
        endPos = position + halfLine;
    } else {
        startPos = trans.position;
        endPos = trans.position + Vector2f::fromAngle(trans.rotation) * static_cast<f32>(line.length);
    }

    return LinePoints{startPos, endPos};
}

namespace rutil {

void trySetShader(Shader* shader) {
    if (shader == nullptr) {
        trySetDefaultShader();
    } else if (shader != S_CURRENT_SHADER) {
        shader->bind();
        S_CURRENT_SHADER = shader;
    }
}

void trySetDefaultShader() {
    if (S_CURRENT_SHADER != nullptr) {
        assert(S_DEFAULT_SHADER != nullptr);
        S_DEFAULT_SHADER->bind();
        S_CURRENT_SHADER = nullptr;
    }
}

}  // namespace rutil
}  // namespace whal

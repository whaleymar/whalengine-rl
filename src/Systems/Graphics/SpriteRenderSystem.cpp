#include "SpriteRenderSystem.h"

#include "Common.h"
#include "Components/Draw.h"
#include "Components/Tags.h"
#include "Components/Transform.h"
#include "Gfx/ShaderManager.h"
#include "Gfx/Texture.h"
#include "Physics/Box.h"

#include "Util/Print.h"
#include "rlgl.h"

namespace whal {

// TODO put in raylib utils
// This overwrites the `vertexNormal` attribute with a custom HDR color value
// This calls a customized version of `rlColor4f` which uses full precision colors
// I can also co-opt the normals RESEARCH
// In the Future Future I should just change the raylib batched vertex buffer to support more custom stuff
static void DrawSpriteHDR(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint, Vector3 hdrColor);

void SpriteRenderSystem::draw(const gfx::EntityRenderInfo& eCtx, const gfx::RenderContext& ctx) const {
    const auto sprite = eCtx.entity.get<Sprite>();
    const auto frameSize = sprite.frameSize.as<f32>();

    const s32 flipModifier = eCtx.preciseTransform.facing == Facing::Left ? -1 : 1;
    const Rectangle srcRect = Rectangle(sprite.atlasPosition.x, sprite.atlasPosition.y, flipModifier * frameSize.x, frameSize.y);
    gfx::RaylibDrawParams params = gfx::getDrawParams(eCtx.preciseTransform, frameSize, ctx.cameraPosition);

    if (ctx.colorOverride) {
        DrawTexturePro(ctx.atlas.getTexture(), srcRect, params.rect, params.origin, eCtx.preciseTransform.rotationDegrees, *ctx.colorOverride);
        return;
    }
    // If we don't deactivate, we minimize the number of shader swaps.
    // Swaps only happen if the passed shader isn't the active one.
    Shader shader = ShaderManager::get(Shaders::Default);
    BeginShaderMode(shader);
    Vector3 hdrCol;
    if (!ctx.colorOverride && eCtx.entity.has<Player>()) {
        static f32 lum = 1.0;
        if (IsKeyPressed(KEY_RIGHT)) {
            lum += 0.1;
            print(lum);
        } else if (IsKeyPressed(KEY_LEFT)) {
            lum -= 0.1;
            print(lum);
        }
        hdrCol = {1.0f * lum, 1.0f * lum, 1.0f * lum};
        // hdrCol = {1.0, 1.0, 1.0};
    } else {
        Vector4 col4 = ColorNormalize(sprite.color);
        hdrCol = {col4.x, col4.y, col4.z};
        // hdrCol = {1.0, 1.0, 1.0};
    }
    // DrawTexturePro(ctx.atlas.getTexture(), srcRect, params.rect, params.origin, eCtx.preciseTransform.rotationDegrees, color);
    DrawSpriteHDR(ctx.atlas.getTexture(), srcRect, params.rect, params.origin, eCtx.preciseTransform.rotationDegrees, sprite.color, hdrCol);
}

void SpriteRenderSystem::addToQueue(std::vector<gfx::EntityRenderInfo>& queue) const {
    // TODO put .reserve in Renderer
    queue.reserve(getEntitiesMutable().size());  // reserve space in case capacity is too low
    for (auto [entityid, entity] : getEntitiesMutable()) {
        const auto sprite = entity.get<Sprite>();
        const auto& trans = entity.get<Transform2D>();
        const auto bb = trans.rotationDegrees == 0.0f ?
                            AABB(trans, sprite.frameSize / 2, Vector2i()) :
                            Box(trans.getRotatedPosition(), sprite.frameSize / 2, trans.rotationDegrees).getBoundingAABB();

        queue.emplace_back(gfx::EntityRenderInfo{
            .boundingBox = bb,
            .preciseTransform = gfx::getPreciseTrans(entity, trans),
            .entity = entity,
            .piRender = this,
        });
    }
}

void DrawSpriteHDR(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint, Vector3 hdrColor) {
    // Check if texture is valid
    if (texture.id > 0) {
        float width = (float)texture.width;
        float height = (float)texture.height;

        bool flipX = false;

        if (source.width < 0) {
            flipX = true;
            source.width *= -1;
        }
        if (source.height < 0)
            source.y -= source.height;

        Vector2 topLeft;
        Vector2 topRight;
        Vector2 bottomLeft;
        Vector2 bottomRight;

        // Only calculate rotation if needed
        if (rotation == 0.0f) {
            float x = dest.x - origin.x;
            float y = dest.y - origin.y;
            topLeft = (Vector2){x, y};
            topRight = (Vector2){x + dest.width, y};
            bottomLeft = (Vector2){x, y + dest.height};
            bottomRight = (Vector2){x + dest.width, y + dest.height};
        } else {
            float sinRotation = sinf(rotation * DEG2RAD);
            float cosRotation = cosf(rotation * DEG2RAD);
            float x = dest.x;
            float y = dest.y;
            float dx = -origin.x;
            float dy = -origin.y;

            topLeft.x = x + dx * cosRotation - dy * sinRotation;
            topLeft.y = y + dx * sinRotation + dy * cosRotation;

            topRight.x = x + (dx + dest.width) * cosRotation - dy * sinRotation;
            topRight.y = y + (dx + dest.width) * sinRotation + dy * cosRotation;

            bottomLeft.x = x + dx * cosRotation - (dy + dest.height) * sinRotation;
            bottomLeft.y = y + dx * sinRotation + (dy + dest.height) * cosRotation;

            bottomRight.x = x + (dx + dest.width) * cosRotation - (dy + dest.height) * sinRotation;
            bottomRight.y = y + (dx + dest.width) * sinRotation + (dy + dest.height) * cosRotation;
        }

        rlSetTexture(texture.id);
        rlBegin(RL_QUADS);

        rlColor4f(hdrColor.x, hdrColor.y, hdrColor.z, static_cast<f32>(tint.a) / 255.0f);
        // rlColor4ub(tint.r, tint.g, tint.b, tint.a); // old way, LDR colors
        rlNormal3f(hdrColor.x, hdrColor.y, hdrColor.z);
        // rlSetNormals(hdrColor);

        // Top-left corner for texture and quad
        if (flipX)
            rlTexCoord2f((source.x + source.width) / width, source.y / height);
        else
            rlTexCoord2f(source.x / width, source.y / height);
        rlVertex2f(topLeft.x, topLeft.y);

        // Bottom-left corner for texture and quad
        if (flipX)
            rlTexCoord2f((source.x + source.width) / width, (source.y + source.height) / height);
        else
            rlTexCoord2f(source.x / width, (source.y + source.height) / height);
        rlVertex2f(bottomLeft.x, bottomLeft.y);

        // Bottom-right corner for texture and quad
        if (flipX)
            rlTexCoord2f(source.x / width, (source.y + source.height) / height);
        else
            rlTexCoord2f((source.x + source.width) / width, (source.y + source.height) / height);
        rlVertex2f(bottomRight.x, bottomRight.y);

        // Top-right corner for texture and quad
        if (flipX)
            rlTexCoord2f(source.x / width, source.y / height);
        else
            rlTexCoord2f((source.x + source.width) / width, source.y / height);
        rlVertex2f(topRight.x, topRight.y);

        rlEnd();
        rlSetTexture(0);
    }
}

}  // namespace whal

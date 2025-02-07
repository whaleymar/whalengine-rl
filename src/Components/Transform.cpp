#include "Transform.h"

#include <rfl/json.hpp>

#include "Gfx/Color.h"
#include "Gfx/Coordinates.h"
#include "Gfx/RaylibUtil.h"
#include "Settings.h"
#include "Util/CameraUtil.h"

namespace whal {

static Vector2f _getRotatedPosition(Vector2f position, Vector2f scale, Vector2f pivotOffset, f32 rotationDegrees, f32 floatHeight) {
    scale = scale.absolute();
    if (rotationDegrees == 0.0f || pivotOffset.isZero()) {
        return position + Vector2f(0, floatHeight * FLOAT_HEIGHT_MULT) + pivotOffset * (Vector2f::ONE - scale);
    }
    const Vector2f pivotRoot = position + pivotOffset;
    const Vector2f unscaled = position.rotate(rotationDegrees, pivotRoot) + Vector2f(0, floatHeight * FLOAT_HEIGHT_MULT);
    const Vector2f delta = unscaled - pivotRoot;
    return pivotRoot + delta * scale;
}

Transform Transform::world(s32 x, s32 y) {
    Vector2f pos(x, y);
    return Transform{.position = pos, .positionPx = pos.as<s32>(), ._localPosition = pos};
}

Transform Transform::world(Vector2i pos) {
    return Transform{.position = pos.as<f32>(), .positionPx = pos, ._localPosition = pos.as<f32>()};
}

Transform Transform::world(Vector2f pos) {
    return Transform{.position = pos, .positionPx = pos.round(), ._localPosition = pos};
}

Transform Transform::tiles(s32 x, s32 y) {
    Vector2f pos(x * PIXELS_PER_TILE, y * PIXELS_PER_TILE);
    return Transform{.position = pos, .positionPx = pos.as<s32>(), ._localPosition = pos};
}

Transform Transform::tiles(Vector2i pos) {
    Vector2f posF = Vector2f(pos.x * PIXELS_PER_TILE, pos.y * PIXELS_PER_TILE);
    return Transform{.position = posF, .positionPx = posF.as<s32>(), ._localPosition = posF};
}

void Transform::translate(Vector2f moveAmount, ecs::Entity self) {
    position += moveAmount;
    positionPx = position.round();
    _localPosition += moveAmount;
    isDirty = true;
    for (const ecs::Entity& child : self.children()) {
        child.get<Transform>().setParentPosition(getRotatedPosition(), child);
    }
}

void Transform::rotate(f32 degrees, ecs::Entity self) {
    rotation += degrees;
    _localRotation += degrees;
    // isDirty = true; // RESEARCH
    for (const ecs::Entity& child : self.children()) {
        child.get<Transform>().setParentRotation(rotation, child);
    }
}

void Transform::scaleBy(Vector2f amount, ecs::Entity self) {
    scale *= amount;
    _localScale *= amount;
    for (const ecs::Entity& child : self.children()) {
        child.get<Transform>().setParentScale(scale, child);
    }
}

void Transform::setParent(const Transform& parentTrans, ecs::Entity self) {
    // rotate about parent's center
    if (!_localPosition.isZero()) {
        const f32 oldParentRotation = rotation - _localRotation;
        _localPosition = _localPosition.rotate(parentTrans.rotation - oldParentRotation, Vector2f::ZERO);
    }

    // TODO i think I need to do something similar to ^ if there's a pivot offset
    // but it's really confusing
    // this might be a good excuse to abandon pivot offsets in favor of another approach
    // if (!pivotOffset.isZero()) {
    //     const f32 oldParentRotation = rotation - localRotation;
    //     localPosition = pivotOffset.rotate(parentTrans.rotation - oldParentRotation, Vector2f::ZERO);
    // }

    position = parentTrans.getRotatedPosition() + _localPosition;  // handles floating height
    positionPx = position.round();
    scale = parentTrans.scale * _localScale;
    rotation = parentTrans.rotation + _localRotation;
    isDirty = true;
    // depth = parentTrans.depth; // annoying
    for (const ecs::Entity& child : self.children()) {
        child.get<Transform>().setParent(*this, child);
    }
}

void Transform::setParentPosition(Vector2f parentPositionTransformed, ecs::Entity self) {
    position = parentPositionTransformed + _localPosition;
    positionPx = position.round();
    isDirty = true;
    for (const ecs::Entity& child : self.children()) {
        child.get<Transform>().setParentPosition(getRotatedPosition(), child);
    }
}

void Transform::setParentScale(Vector2f parentScale, ecs::Entity self) {
    scale = parentScale * _localScale;
    for (const ecs::Entity& child : self.children()) {
        child.get<Transform>().setParentScale(scale, child);
    }
}

void Transform::setParentRotation(f32 parentDegrees, ecs::Entity self) {
    // rotate about parent's center
    if (!_localPosition.isZero()) {
        const f32 oldParentRotation = rotation - _localRotation;
        const Vector2f oldLocalPos = _localPosition;
        _localPosition = _localPosition.rotate(parentDegrees - oldParentRotation, Vector2f::ZERO);
        position += (_localPosition - oldLocalPos);
        positionPx = position.round();
    }
    rotation = parentDegrees + _localRotation;
    // isDirty = true; // RESEARCH
    for (const ecs::Entity& child : self.children()) {
        child.get<Transform>().setParentRotation(rotation, child);
    }
}

void Transform::set(const Transform& trans, ecs::Entity self) {
    const Vector2f parentPosition = position - _localPosition;
    position = trans.position;
    positionPx = position.round();
    _localPosition = position - parentPosition;

    if (trans.scale != scale) {
        const Vector2f parentScale = self.parent().isValid() ? self.parent().get<Transform>().scale : Vector2f::ONE;
        scale = trans.scale;
        if (parentScale.x == 0) {
            _localScale.x = scale.x;
        } else {
            _localScale.x = scale.x / parentScale.x;
        }
        if (parentScale.y == 0) {
            _localScale.y = scale.y;
        } else {
            _localScale.y = scale.y / parentScale.y;
        }
    }

    const f32 parentRotation = rotation - _localRotation;
    rotation = trans.rotation;
    _localRotation = rotation - parentRotation;

    floatHeight = trans.floatHeight;
    isDirty = true;
    depth = trans.depth;
    pivotOffset = trans.pivotOffset;

    for (const ecs::Entity& child : self.children()) {
        child.get<Transform>().setParent(*this, child);
    }
}

void Transform::setPosition(Vector2f globalPosition, ecs::Entity self) {
    if (globalPosition == position) {
        return;
    }

    const Vector2f parentPosition = position - _localPosition;
    position = globalPosition;
    positionPx = position.round();
    _localPosition = position - parentPosition;

    isDirty = true;
    for (const ecs::Entity& child : self.children()) {
        child.get<Transform>().setParentPosition(getRotatedPosition(), child);
    }
}

void Transform::setScale(Vector2f globalScale, ecs::Entity self) {
    if (globalScale == scale) {
        return;
    }

    // do some schenanigans to avoid Divide-By-Zero
    const Vector2f parentScale = self.parent().isValid() ? self.parent().get<Transform>().scale : Vector2f::ONE;
    scale = globalScale;
    if (parentScale.x == 0) {
        _localScale.x = scale.x;
    } else {
        _localScale.x = scale.x / parentScale.x;
    }
    if (parentScale.y == 0) {
        _localScale.y = scale.y;
    } else {
        _localScale.y = scale.y / parentScale.y;
    }

    for (const ecs::Entity& child : self.children()) {
        child.get<Transform>().setParentScale(scale, child);
    }
}

void Transform::setRotation(f32 globalRotation, ecs::Entity self) {
    if (globalRotation == rotation) {
        return;
    }

    const f32 parentRotation = rotation - _localRotation;
    rotation = globalRotation;
    _localRotation = rotation - parentRotation;
    // isDirty = true; // RESEARCH
    if (pivotOffset.isZero()) {
        for (const ecs::Entity& child : self.children()) {
            child.get<Transform>().setParentRotation(rotation, child);
        }
    } else {
        // if we rotated about a pivot, we need to update child positions as well
        const Vector2f transformedPos = getRotatedPosition();
        for (const ecs::Entity& child : self.children()) {
            auto& childTrans = child.get<Transform>();
            childTrans.setParentRotation(rotation, child);
            childTrans.setParentPosition(transformedPos, child);
        }
    }
}

void Transform::setFloatHeight(f32 globalFloatHeight, ecs::Entity self) {
    if (globalFloatHeight == floatHeight) {
        return;
    }

    // local float height is not a thing
    floatHeight = globalFloatHeight;
    for (const ecs::Entity& child : self.children()) {
        child.get<Transform>().setParentPosition(getRotatedPosition(), child);
    }
}

void Transform::setFacing(Facing dir, ecs::Entity self) {
    bool isDirChange = (dir == Facing::Left && scale.x > 0.0f) || (dir == Facing::Right && scale.x < 0.0f);
    if (isDirChange) {
        scaleBy(Vector2f(-1.0f, 1.0f), self);
    }
}

Vector2f Transform::getRotatedPosition() const {
    return _getRotatedPosition(position, scale, pivotOffset, rotation, floatHeight);
}

Vector2i Transform::getRotatedPositionInt() const {
    return _getRotatedPosition(positionPx.as<f32>(), scale, pivotOffset, rotation, floatHeight).round();
}

Vector2f Transform::apply(Vector2f relOffset) const {
    // optimize for most common case
    if (rotation == 0.0) {
        const auto scaleAdjustment = (pivotOffset * (Vector2f::ONE - scale.absolute()));
        return position + Vector2f(0, floatHeight * FLOAT_HEIGHT_MULT) + relOffset + scaleAdjustment;
    }

    // RESEARCH might want to use fast variants of these functions
    const Vector2f rotatedOffset = relOffset.isZero() ? Vector2f::ZERO : relOffset.as<f32>().rotate(rotation, Vector2f::ZERO);
    return getRotatedPosition() + rotatedOffset;
}

Vector2i Transform::apply(Vector2i relOffset) const {
    // optimize for most common case
    if (rotation == 0.0) {
        const auto scaleAndFloatAdjustment =
            (pivotOffset * (Vector2f::ONE - scale.absolute()) + Vector2f(0, floatHeight * FLOAT_HEIGHT_MULT)).round();
        return positionPx + relOffset + scaleAndFloatAdjustment;
    }

    // RESEARCH might want to use fast variants of these functions
    const Vector2i rotatedOffset = relOffset.isZero() ? Vector2i::ZERO : relOffset.as<f32>().rotate(rotation, Vector2f::ZERO).round();
    return getRotatedPositionInt() + rotatedOffset;
}

Vector2f Transform::applyInverse(Vector2f transformedPosition, Vector2f relOffset) const {
    // optimize for most common case
    if (rotation == 0.0) {
        const auto scaleAdjustment = (pivotOffset * (Vector2f::ONE - scale.absolute()));
        return transformedPosition - Vector2f(0, floatHeight * FLOAT_HEIGHT_MULT) - relOffset - scaleAdjustment;
    }

    // RESEARCH might want to use fast variants of these functions
    const Vector2f rotatedOffset = relOffset.isZero() ? Vector2f::ZERO : relOffset.as<f32>().rotate(rotation, Vector2f::ZERO);
    const Vector2f transformation = getRotatedPosition() + rotatedOffset;
    return transformedPosition - (transformation - this->position);
}

Vector2i Transform::applyInverse(Vector2i transformedPosition, Vector2i relOffset) const {
    // optimize for most common case
    if (rotation == 0.0) {
        const auto scaleAndFloatAdjustment =
            (pivotOffset * (Vector2f::ONE - scale.absolute()) + Vector2f(0, floatHeight * FLOAT_HEIGHT_MULT)).round();
        return transformedPosition - relOffset - scaleAndFloatAdjustment;
    }

    // RESEARCH might want to use fast variants of these functions
    const Vector2i rotatedOffset = relOffset.isZero() ? Vector2i::ZERO : relOffset.as<f32>().rotate(rotation, Vector2f::ZERO).round();
    const Vector2i transformation = getRotatedPositionInt() + rotatedOffset;
    return transformedPosition - (transformation - this->positionPx);
}

std::string Transform::saveImpl(ecs::Entity entity) {
    return rfl::json::write(entity.get<Transform>());
}

#ifndef NDEBUG
// Draws the root position + transformed root, according to the rotation + scale + pivot
void Transform::draw() const {
    gfx::DrawPixel(worldToRenderCoords(position), Colors::Red);
    auto unrounded = _getRotatedPosition(position, scale, pivotOffset, rotation, 0.0f);
    gfx::DrawPixel(worldToRenderCoords(unrounded), Colors::Green);
}
#endif

TransformBuilder::TransformBuilder(const Transform& trans) : mTrans(trans) {}

TransformBuilder::TransformBuilder(ecs::Entity entity) : mTrans(entity.get<Transform>()) {}

TransformBuilder& TransformBuilder::translate(Vector2f moveAmount) {
    mTrans.position += moveAmount;
    mTrans._localPosition += moveAmount;
    mTrans.positionPx = mTrans.position.round();
    return *this;
}

TransformBuilder& TransformBuilder::scaleBy(Vector2f mult) {
    mTrans.scale *= mult;
    mTrans._localScale *= mult;
    return *this;
}

TransformBuilder& TransformBuilder::rotate(f32 degrees) {
    mTrans.rotation += degrees;
    mTrans._localRotation += degrees;
    return *this;
}

TransformBuilder& TransformBuilder::position(Vector2f globalPosition) {
    const Vector2f parentPosition = mTrans.position - mTrans._localPosition;
    mTrans.position = globalPosition;
    mTrans.positionPx = mTrans.position.round();
    mTrans._localPosition = mTrans.position - parentPosition;
    return *this;
}

TransformBuilder& TransformBuilder::scale(Vector2f globalScale) {
    // avoid DBZ:
    const Vector2f parentScale(mTrans._localScale.x == 0 ? 0 : mTrans.scale.x / mTrans._localScale.x,
                               mTrans._localScale.y == 0 ? 0 : mTrans.scale.y / mTrans._localScale.y);

    mTrans.scale = globalScale;
    mTrans._localScale = Vector2f(parentScale.x == 0 ? 0 : mTrans.scale.x / parentScale.x, parentScale.y == 0 ? 0 : mTrans.scale.y / parentScale.y);
    return *this;
}

TransformBuilder& TransformBuilder::rotation(f32 globalRotation) {
    const f32 parentRotation = mTrans.rotation - mTrans._localRotation;
    mTrans.rotation = globalRotation;
    mTrans._localRotation = mTrans.rotation - parentRotation;
    return *this;
}

TransformBuilder& TransformBuilder::height(f32 height) {
    mTrans.floatHeight = height;
    return *this;
}

TransformBuilder& TransformBuilder::depth(Depth depth) {
    mTrans.depth = depth;
    return *this;
}
TransformBuilder& TransformBuilder::facing(Facing dir) {
    bool isDirChange = (dir == Facing::Left && mTrans.scale.x > 0.0f) || (dir == Facing::Right && mTrans.scale.x < 0.0f);
    if (isDirChange) {
        mTrans.scale.x *= -1.0f;
    }
    return *this;
}

Transform TransformBuilder::build() const {
    return mTrans;
}

}  // namespace whal

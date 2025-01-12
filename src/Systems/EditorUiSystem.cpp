#ifndef NDEBUG

#include "EditorUiSystem.h"

#include "imgui.h"
#include "raylib.h"

#include "Components/Name.h"
#include "Components/Transform.h"

#include "Events/Events.h"
#include "Gfx/Coordinates.h"
#include "Physics/QuadTree/Quadtree.h"
#include "Settings.h"
#include "Util/CameraUtil.h"

namespace whal {

qtree::QuadTree makeQuadTree() {
    return qtree::QuadTree(
        AABB(Vector2i(WINDOW_WIDTH_RENDER / 2, WINDOW_HEIGHT_RENDER / 2), Vector2i(WINDOW_WIDTH_RENDER / 2, WINDOW_HEIGHT_RENDER / 2)));
}

static qtree::QuadTree S_QTREE = makeQuadTree();
static bool S_IS_ACTIVE = false;
static Vector2f S_CAMERA_POS;
static bool S_TREE_INVALID = false;

// TODO feature list:
// 1) Display more components in imgui
// 2) Make serializable components editable in imgui
// 3) entities with a sprite have a bounding box matching that sprite

static AABB getUiBox(Vector2f worldPosition, Vector2i halflen = Vector2i::ZERO) {
    const Vector2f cameraPos = getCameraPositionPrecise();
    if (halflen.isZero()) {
        halflen = Vector2i(PIXELS_PER_TILE / 2 * VIRTUAL_SCREEN_RATIO, PIXELS_PER_TILE / 2 * VIRTUAL_SCREEN_RATIO);
    }

    Vector2i screenPos = worldToScreenCoords(worldPosition, cameraPos);
    screenPos = Vector2i(screenPos.x, WINDOW_HEIGHT_RENDER - screenPos.y);  // idk why i only have to do this here
    return AABB(screenPos, halflen);
}

static AABB getUiBox(ecs::Entity entity) {
    return getUiBox(entity.get<Transform>().position);
}

void EditorUiSystem::buildEntityTree() const {
    S_QTREE = makeQuadTree();  // clears it
    // add everything with a transform inside the screen to the quad tree
    // this is imperfect for sprites partially on screen but idc
    for (auto [entityid, entity] : getEntities()) {
        const AABB uiBox = getUiBox(entity);
        if (S_QTREE.getBoundingBox().contains(uiBox)) {
            // make each bounding box a single tile
            // ideally it should match the entity's sprite if they have one, but then it would need to update when those components changed... lots of
            // work
            S_QTREE.add(entity, uiBox);
        }
    }
    S_TREE_INVALID = false;
}

void EditorUiSystem::activate() {
    assert(System::isQuietPaused() && EDITOR_MODE && "Must be paused and in editor mode to activate Editor UI");
    S_IS_ACTIVE = true;
    S_CAMERA_POS = getCameraPositionPrecise();
    buildEntityTree();
}

void EditorUiSystem::deactivate() {
    S_IS_ACTIVE = false;
    mClickedEntities.clear();
    ecs::Entity camera = *getCamera();
    camera.get<Transform>().setPosition(S_CAMERA_POS, camera);
}

void EditorUiSystem::dragSelectedEntities() const {
    const Vector2f delta = rl::GetMouseDelta();
    if (delta.isZero() || mClickedEntities.size() == 0) {
        return;
    }

    for (ecs::Entity selected : mClickedEntities) {
        const Vector2f worldDelta = delta * Vector2f(1, -1) / VIRTUAL_SCREEN_RATIO;
        const AABB oldUiBox = getUiBox(selected);
        S_QTREE.remove(selected, oldUiBox);
        selected.get<Transform>().translate(worldDelta, selected);
        const AABB newUiBox = getUiBox(selected);
        if (S_QTREE.getBoundingBox().contains(newUiBox)) {
            S_QTREE.add(selected, newUiBox);
        } else {
            // undo the move
            selected.get<Transform>().translate(worldDelta * -1, selected);
            S_QTREE.add(selected, oldUiBox);
        }
    }
}

void EditorUiSystem::panCamera() const {
    const Vector2f delta = rl::GetMouseDelta();
    if (delta.isZero()) {
        return;
    }

    ecs::Entity camera = *getCamera();
    camera.get<Transform>().translate(delta * Vector2f(-1, 1) / VIRTUAL_SCREEN_RATIO, camera);

    // since the camera moved, the screen space tree coords are invalid
    // should I just make it be in world space or something?
    S_TREE_INVALID = true;
}

static std::vector<ecs::Entity> pruneChildren(const std::vector<ecs::Entity>& entities) {
    std::unordered_set<ecs::Entity, ecs::EntityHash> lut;
    for (ecs::Entity e : entities) {
        lut.insert(e);
    }
    const auto isDescendant = [&](const ecs::Entity& child) -> bool {
        ecs::Entity current = child.parent();
        while (current.isValid()) {
            if (lut.find(current) != lut.end()) {
                return true;
            }
            current = current.parent();
        }
        return false;
    };

    std::vector<ecs::Entity> result;
    for (ecs::Entity e : entities) {
        if (!isDescendant(e)) {
            result.push_back(e);
        }
    }
    return result;
}

void EditorUiSystem::onEvent(evt::Input, InputEvent input) {
    if (!S_IS_ACTIVE) {
        return;
    }

    if (input.name == "ui_altselect") {
        if (input.isHeld) {
            panCamera();
        }
        return;
    } else if (input.name != "ui_select") {
        return;
    }

    if (S_TREE_INVALID) {
        buildEntityTree();
    }

    if (input.isHeld) {
        dragSelectedEntities();
        return;
    } else if (!input.isPressed) {
        return;
    }

    Vector2i clickPoint = Input.getMouseScreen();
    AABB queryBox = AABB(clickPoint, Vector2i(1, 1));

    // If this list has an entity that's a parent of another entity in this list, BAD
    // Cause then if i move everything in the list, the child gets moved double
    // So I'll just keep the parent, and have the child available via the parent
    mClickedEntities = pruneChildren(S_QTREE.query(queryBox));
}

void EditorUiSystem::onEvent(evt::EnginePause, bool isPaused) {
    if (isPaused) {
        activate();
    } else {
        deactivate();
    }
}

void EditorUiSystem::onEvent(evt::WindowResize) {
    S_TREE_INVALID = true;
}

void EditorUiSystem::onAdd(ecs::Entity entity) {
    if (!S_IS_ACTIVE) {
        return;
    }
    if (S_TREE_INVALID) {
        buildEntityTree();
    }
    const AABB uiBox = getUiBox(entity);
    if (S_QTREE.getBoundingBox().contains(uiBox)) {
        S_QTREE.add(entity, getUiBox(entity));
    }
}

void EditorUiSystem::onRemove(ecs::Entity entity) {
    if (!S_IS_ACTIVE) {
        return;
    }
    if (S_TREE_INVALID) {
        buildEntityTree();
    }
    const AABB uiBox = getUiBox(entity);
    if (S_QTREE.getBoundingBox().contains(uiBox)) {
        S_QTREE.remove(entity, getUiBox(entity));
    }
}

static void printComponents(ecs::Entity entity, const std::string prefix = "") {
    ImGui::Text("%sName: %s\n%sTransform: %s\n", prefix.c_str(), entity.has<Name>() ? entity.get<Name>().name.c_str() : "None", prefix.c_str(),
                Transform::saveImpl(entity).c_str());
    entity.forChild(&printComponents, true, prefix + "\t");
}

void EditorUiSystem::draw() {
    if (!S_IS_ACTIVE) {
        return;
    }

    for (ecs::Entity entity : mClickedEntities) {
        Transform trans = entity.get<Transform>();
        // I'm drawing to the window (not to a render texture with the correct resolution), so I have to correct for this:
        if (rl::GetRenderWidth() != WINDOW_WIDTH_RENDER || rl::GetRenderHeight() != WINDOW_HEIGHT_RENDER) {
            // the window size is mismatched for some reason, make sure we're drawing it centered
            trans.position.x += static_cast<f32>((rl::GetRenderWidth() - WINDOW_WIDTH_RENDER) / 2) / VIRTUAL_SCREEN_RATIO;
            trans.position.y -= static_cast<f32>((rl::GetRenderHeight() - WINDOW_HEIGHT_RENDER) / 2) / VIRTUAL_SCREEN_RATIO;
            trans.positionPx = trans.position.round();
        }

        trans.draw();
        AABB(trans, Vector2i(PIXELS_PER_TILE / 2, PIXELS_PER_TILE / 2)).draw(Colors::Green);
        printComponents(entity);
    }
}

}  // namespace whal

#endif

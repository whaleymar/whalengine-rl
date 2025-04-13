#include "Sys/Time.h"
#ifndef NDEBUG

#include "Components/Collider.h"
#include "Components/ComponentReflection.h"
#include "Components/Transform.h"
#include "ECS.h"
#include "EditorUiSystem.h"
#include "Events/Events.h"
#include "Gfx/Coordinates.h"
#include "IGame.h"
#include "InspectorComponents.h"  // DEFINED IN GAME REPO
#include "Map/Level.h"
#include "Settings.h"
#include "Sys/InputHandler.h"
#include "Systems/ColliderSystem.h"
#include "Util/CameraUtil.h"
#include "imgui.h"
#include "raylib.h"
#include "rlImGuiColors.h"

namespace whal {

// static const AABB S_SCREEN_BOX =
//     AABB(Vector2i(WINDOW_WIDTH_STRETCH / 2, WINDOW_HEIGHT_STRETCH / 2), Vector2i(WINDOW_WIDTH_STRETCH / 2, WINDOW_HEIGHT_STRETCH / 2));
static bool S_IS_ACTIVE = false;
static Vector2f S_CAMERA_POS;

static AABB getUiBox(Vector2f worldPosition, Vector2i halflen = Vector2i::ZERO) {
    const Vector2f cameraPos = getCameraPositionPrecise();
    if (halflen.isZero()) {
        // make the uiBox 1 tile (imperfect)
        halflen = Vector2f(FPIXELS_PER_TILE / 2.0 * VIRTUAL_SCREEN_RATIO_STRETCH, FPIXELS_PER_TILE / 2.0 * VIRTUAL_SCREEN_RATIO_STRETCH).ceil();
    }

    Vector2i screenPos = worldToScreenCoords(worldPosition, cameraPos, ScreenResolution::Stretched);
    screenPos = Vector2i(screenPos.x, WINDOW_HEIGHT_STRETCH - screenPos.y);  // idk why i only have to do this here
    return AABB(screenPos, halflen);
}

static AABB getUiBox(ecs::Entity entity) {
    if (entity.has<Sprite>()) {
        const Sprite& sprite = entity.get<Sprite>();
        Vector2i customSize = sprite.getFrame().size / 2;
        customSize = (customSize.as<f32>() * VIRTUAL_SCREEN_RATIO_STRETCH).round();
        return getUiBox(entity.get<Transform>().getRotatedPosition(), customSize);
    } else {
        return getUiBox(entity.get<Transform>().getRotatedPosition());
    }
}

void EditorUiSystem::activate() {
    assert(System::isQuietPaused() && EDITOR_MODE && EDITOR_SUSPEND && "Must be paused and in editor mode to activate Editor UI");
    S_IS_ACTIVE = true;
    S_CAMERA_POS = getCameraPositionPrecise();
}

void EditorUiSystem::deactivate() {
    S_IS_ACTIVE = false;
    ecs::Entity camera = getCamera();
    camera.get<Transform>().setPosition(S_CAMERA_POS, camera);
}

void EditorUiSystem::dragSelectedEntities() const {
    const Vector2f delta = rl::GetMouseDelta();
    if (delta.isZero() || mClickedEntities.size() == 0) {
        return;
    }

    for (ecs::Entity selected : mClickedEntities) {
        const Vector2f worldDelta = delta * Vector2f(1, -1) / VIRTUAL_SCREEN_RATIO_STRETCH;
        selected.get<Transform>().translate(worldDelta, selected);
        if (selected.has<Collider>()) {
            // update collider (physics system is turned off during engine pause)
            auto& collider = selected.get<Collider>();
            ColliderSystem::updatePosition(selected, collider.getShapeMutable(), selected.get<Transform>(), collider.getOffset());
        }
    }
}

void EditorUiSystem::panCamera() const {
    const Vector2f delta = rl::GetMouseDelta();
    if (delta.isZero()) {
        return;
    }

    ecs::Entity camera = getCamera();
    camera.get<Transform>().translate(delta * Vector2f(-1, 1) / VIRTUAL_SCREEN_RATIO_STRETCH, camera);
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

    if (input.isHeld) {
        // don't accidentally move for a short click
        if (Time.getElapsedPrecise() - mLastClickTime > 0.1) {
            dragSelectedEntities();
        }
        return;
    } else if (!input.isPressed) {
        return;
    }

    // if click was outside of game window, do nothing (want to maintain clicked entity list)
    if (Input.getMouseScreen().x > WINDOW_WIDTH_STRETCH || Input.getMouseScreen().y > WINDOW_HEIGHT_STRETCH) {
        return;
    }

    mLastClickTime = Time.getElapsedPrecise();

    // build mClickedEntities
    Vector2i clickPoint = Input.getMouseScreen();
    AABB queryBox = AABB(clickPoint, Vector2i(1, 1));
    std::vector<ecs::Entity> prevClickedEntities = mClickedEntities;
    mClickedEntities.clear();
    for (auto [entityid, entity] : getEntities()) {
        const AABB uiBox = getUiBox(entity);
        if (uiBox.isOverlapping(queryBox)) {
            mClickedEntities.push_back(entity);
        }
    }

    // if nothing was clicked, keep old selection
    if (mClickedEntities.empty()) {
        mClickedEntities = prevClickedEntities;
    }

    // If this list has an entity that's a parent of another entity in this list, BAD
    // Cause then if i move everything in the list, the child gets moved double
    // So I'll just keep the parent, and have the child available via the parent
    mClickedEntities = pruneChildren(mClickedEntities);
}

void EditorUiSystem::onEvent(evt::EnginePause, bool isPaused) {
    if (isPaused) {
        activate();
    } else {
        deactivate();
    }
}

void EditorUiSystem::onEvent(evt::Restart, bool resetPlayers) {
    mClickedEntities.clear();
}

void EditorUiSystem::onAdd(ecs::Entity entity) {}

void EditorUiSystem::onRemove(ecs::Entity entity) {
    // remove entity from clicked list
    auto it = ecs::whal_find(mClickedEntities.begin(), mClickedEntities.end(), entity);
    if (it != mClickedEntities.end()) {
        print("removed entity id", entity.id(), "from clickedEntityies because DIE");
        mClickedEntities.erase(it);
    }
}

void EditorUiSystem::drawDebug() {
    for (ecs::Entity entity : mClickedEntities) {
        Transform trans = entity.get<Transform>();

        trans.draw();  // also hard-coded for render window size
        AABB(trans.apply(Vector2i::ZERO), getUiBox(entity).getHalf() / VIRTUAL_SCREEN_RATIO_STRETCH).draw(Colors::White);
    }
}

template <typename T>
static void componentEditor(ecs::Entity entity) {
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
    if constexpr (std::is_same_v<T, Transform>) {
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }
    if (ImGui::TreeNodeEx(World.component<T>().name(), flags)) {
        if constexpr (IsCustomEditor<T>) {
            entity.get<T>().onEditorRender();
        } else {
            std::pair<T, bool> updated = imguiRenderStruct<T>(entity.get<T>());
            if (updated.second) {
                if constexpr (std::is_same_v<T, Transform>) {
                    // special setter
                    updated.first.isManuallyMoved = true;
                    entity.get<Transform>().set(updated.first, entity);
                } else {
                    entity.set(updated.first);
                }
            }
        }
        ImGui::TreePop();
    }
    // ImGui::EndChild();
}

template <typename Tuple, std::size_t Index = 0>
constexpr void iterComponents(ecs::Entity entity) {
    if constexpr (Index < std::tuple_size_v<Tuple>) {
        using T = std::tuple_element_t<Index, Tuple>;
        if (entity.has<T>()) {
            componentEditor<T>(entity);
        }
        iterComponents<Tuple, Index + 1>(entity);
    }
}

std::string getEntityName(ecs::Entity entity) {
    return whal_format("{} (ID = {})", entity.name(), entity.id());
}

static void drawComponents(ecs::Entity entity, int xOffset = 0) {
    ImGui::PushID(entity.id());
    if (xOffset == 0) {
        xOffset = ImGui::GetCursorPosX();
    } else {
        ImGui::SetCursorPosX(xOffset);
    }
    ImGui::SetCursorPosX(xOffset);
    ImGui::BeginChild(std::to_string(entity.id()).c_str(), ImVec2(0, 0), ImGuiChildFlags_AutoResizeY);
    iterComponents<InspectorComponents>(entity);
    ImGui::EndChild();
    ImGui::Separator();
    for (auto child : entity.children()) {
        ImGui::PushStyleColor(ImGuiCol_Text, rlImGuiColors::Convert(rl::ORANGE));
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (ImGui::TreeNodeEx(getEntityName(child).c_str(), flags)) {
            ImGui::PopStyleColor();
            drawComponents(child, xOffset + 16);
            ImGui::TreePop();
        } else {
            ImGui::PopStyleColor();
        }
    }
    ImGui::PopID();
}

void EditorUiSystem::drawHierarchyRecursive(ecs::Entity rootEntity, const std::vector<ecs::Entity>& openEntities) {
    const ecs::Entity selectedEntity = openEntities.front();
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (rootEntity == selectedEntity) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    auto checkIfEntitySelected = [&]() {
        // updates selected entity on click

        // definition of IsItemClicked:
        // return IsMouseClicked(mouse_button) && IsItemHovered(ImGuiHoveredFlags_None);

        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            mHierachySelectionMouseDown = rootEntity;

        } else if (ImGui::IsMouseReleased(0) && ImGui::IsItemHovered(ImGuiHoveredFlags_None) && !ImGui::IsItemToggledOpen()) {
            // make sure the entity we are setting is the same one selected with mouse down
            if (mHierachySelectionMouseDown == rootEntity) {
                mClickedEntities = {rootEntity};
            }
        }
    };

    if (rootEntity.children().size() == 0) {
        // selectable leaf node
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        ImGui::TreeNodeEx(getEntityName(rootEntity).c_str(), flags);

        checkIfEntitySelected();

    } else {
        auto it = ecs::whal_find(openEntities.begin(), openEntities.end(), rootEntity);
        if (rootEntity != selectedEntity && it != openEntities.end()) {
            // make sure the parent hierarchy above `selectedEntity` is open by default
            // flags |= ImGuiTreeNodeFlags_DefaultOpen; // not persistent
            ImGui::SetNextItemOpen(true, ImGuiCond_Once);  // persistent
        }
        if (ImGui::TreeNodeEx(getEntityName(rootEntity).c_str(), flags)) {
            checkIfEntitySelected();
            for (auto child : rootEntity.children()) {
                drawHierarchyRecursive(child, openEntities);
            }
            ImGui::TreePop();
        } else {
            checkIfEntitySelected();
        }
    }
}

// draws the hierarchy of entities that parent/are children of `entity`
void EditorUiSystem::drawHierarchy(ecs::Entity entity) {
    std::vector<ecs::Entity> parentChain = {entity};
    ecs::Entity current = entity;
    while (true) {
        ecs::Entity parent = current.parent();
        if (parent.isValid()) {
            parentChain.push_back(parent);
            current = parent;
        } else {
            break;
        }
    }

    drawHierarchyRecursive(parentChain.back(), parentChain);
}

// needs to be separate, otherwise the graphical stuff in `draw` will be drawn under the imgui ui
void EditorUiSystem::drawEditor() {
    ImGui::Begin("Inspector");
    for (ecs::Entity entity : mClickedEntities) {
        ImGui::TextColored(rlImGuiColors::Convert(rl::ORANGE), "%s:", getEntityName(entity).c_str());
        drawComponents(entity);
    }
    ImGui::End();  // Inspector

    ImGui::Begin("Hierarchy");
    if (mClickedEntities.size() > 0) {
        drawHierarchy(mClickedEntities[0]);
    } else {
        drawHierarchy(System::getGame().getScene().self);
    }
    ImGui::End();
}

}  // namespace whal

#endif

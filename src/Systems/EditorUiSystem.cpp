#ifndef NDEBUG

#include "EditorUiSystem.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include "ECS.h"
#include "raylib.h"

#include "Components/Collision.h"
#include "Components/Draw.h"
#include "Components/Name.h"
#include "Components/Transform.h"

#include "Events/Events.h"
#include "Gfx/Coordinates.h"
#include "Settings.h"
#include "Util/CameraUtil.h"
#include "rlImGuiColors.h"

#include "Systems/ColliderSystem.h"

namespace whal {

// static const AABB S_SCREEN_BOX =
//     AABB(Vector2i(WINDOW_WIDTH_RENDER / 2, WINDOW_HEIGHT_RENDER / 2), Vector2i(WINDOW_WIDTH_RENDER / 2, WINDOW_HEIGHT_RENDER / 2));
static bool S_IS_ACTIVE = false;
static Vector2f S_CAMERA_POS;

// TODO feature list:
// 1) Display more components in imgui
// 2) Make serializable components editable in imgui
// 3) entities without a sprite have some visual indicator of where they are

static AABB getUiBox(Vector2f worldPosition, Vector2i halflen = Vector2i::ZERO) {
    const Vector2f cameraPos = getCameraPositionPrecise();
    if (halflen.isZero()) {
        // make the uiBox 1 tile (imperfect)
        halflen = Vector2f(FPIXELS_PER_TILE / 2.0 * VIRTUAL_SCREEN_RATIO, FPIXELS_PER_TILE / 2.0 * VIRTUAL_SCREEN_RATIO).ceil();
    }

    Vector2i screenPos = worldToScreenCoords(worldPosition, cameraPos);
    screenPos = Vector2i(screenPos.x, WINDOW_HEIGHT_RENDER - screenPos.y);  // idk why i only have to do this here
    return AABB(screenPos, halflen);
}

static AABB getUiBox(ecs::Entity entity) {
    if (entity.has<Sprite>()) {
        const Sprite& sprite = entity.get<Sprite>();
        Vector2i customSize = sprite.getFrame().size / 2;
        customSize = (customSize.as<f32>() * VIRTUAL_SCREEN_RATIO).round();
        return getUiBox(entity.get<Transform>().position, customSize);
    } else {
        return getUiBox(entity.get<Transform>().position);
    }
}

void EditorUiSystem::activate() {
    assert(System::isQuietPaused() && EDITOR_MODE && EDITOR_SUSPEND && "Must be paused and in editor mode to activate Editor UI");
    S_IS_ACTIVE = true;
    S_CAMERA_POS = getCameraPositionPrecise();
}

void EditorUiSystem::deactivate() {
    S_IS_ACTIVE = false;
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

    ecs::Entity camera = *getCamera();
    camera.get<Transform>().translate(delta * Vector2f(-1, 1) / VIRTUAL_SCREEN_RATIO, camera);
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
    if (Input.getMouseScreen().x > WINDOW_WIDTH_RENDER || Input.getMouseScreen().y > WINDOW_HEIGHT_RENDER) {
        return;
    }

    mLastClickTime = Time.getElapsedPrecise();

    // build mClickedEntities
    Vector2i clickPoint = Input.getMouseScreen();
    AABB queryBox = AABB(clickPoint, Vector2i(1, 1));
    mClickedEntities.clear();
    for (auto [entityid, entity] : getEntities()) {
        const AABB uiBox = getUiBox(entity);
        if (uiBox.isOverlapping(queryBox)) {
            mClickedEntities.push_back(entity);
        }
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

void EditorUiSystem::drawWorld() {
    for (ecs::Entity entity : mClickedEntities) {
        Transform trans = entity.get<Transform>();
        // I'm drawing to the window (not to a render texture with the correct resolution), so I have to correct for this:
        if (WINDOW_POS_OS_X > 0 || WINDOW_POS_OS_Y > 0) {
            // the window size is mismatched for some reason, make sure we're drawing it centered
            trans.position.x += static_cast<f32>(WINDOW_POS_OS_X) / VIRTUAL_SCREEN_RATIO;
            trans.position.y -= static_cast<f32>(WINDOW_POS_OS_Y) / VIRTUAL_SCREEN_RATIO;
            trans.positionPx = trans.position.round();
        }

        trans.draw();
        AABB(trans, getUiBox(entity).getHalf() / VIRTUAL_SCREEN_RATIO).draw(Colors::Green);
    }
}

// Define a type alias to reduce verbosity
template <typename T>
using BaseType = std::remove_cvref_t<std::remove_pointer_t<T>>;

// Define the concept
template <typename T>
concept IsVectorLike =
    requires {
        typename BaseType<T>;  // Ensure the type can be stripped
    } && (std::same_as<BaseType<T>, std::vector<typename T::value_type>> || std::same_as<BaseType<T>, std::deque<typename T::value_type>> ||
          std::same_as<BaseType<T>, std::forward_list<typename T::value_type>> || std::same_as<BaseType<T>, std::list<typename T::value_type>> ||
          std::same_as<BaseType<T>, std::set<typename T::value_type>> || std::same_as<BaseType<T>, std::multiset<typename T::value_type>> ||
          std::same_as<BaseType<T>, std::unordered_set<typename T::value_type>> ||
          std::same_as<BaseType<T>, std::unordered_multiset<typename T::value_type>>);

// Define the concept
template <typename T>
concept IsMapLike =
    requires {
        typename BaseType<T>;  // Ensure the type can be stripped
    } && (std::same_as<BaseType<T>, std::map<typename T::key_type, typename T::mapped_type>> ||
          std::same_as<BaseType<T>, std::multimap<typename T::key_type, typename T::mapped_type>> ||
          std::same_as<BaseType<T>, std::unordered_map<typename T::key_type, typename T::mapped_type>> ||
          std::same_as<BaseType<T>, std::unordered_multimap<typename T::key_type, typename T::mapped_type>>);

// TODO include stdlib types
template <typename T>
concept IsReflectable = std::is_aggregate_v<T> || rfl::internal::has_reflection_type_v<T> || IsVectorLike<T> || IsMapLike<T>;

// using tmpT = Collider;
// bool asdf = IsReflectable<tmpT>;                          // true
// bool asdf2 = std::is_aggregate_v<tmpT>;                   // false
// bool asdf3 = rfl::internal::has_reflection_type_v<tmpT>;  // false

// bool tracks if anything changed
template <typename T>
static std::pair<T, bool> imguiRenderStruct(T thing, const std::string& prefix = "", const std::string& ignoreFieldsWithPrefix = "_") {
    if constexpr (rfl::internal::has_reflection_type_v<T>) {
        // make sure we use the reflectable type if it has one
        return imguiRenderStruct(thing.reflection(), prefix, ignoreFieldsWithPrefix);
    } else {
        const auto tup = rfl::to_view(thing);
        bool isChange = false;
        tup.apply([&]<typename Field>(Field& f) {
            using Dtype = std::remove_reference_t<decltype(*f.value())>;
            const std::string field_name = std::string(Field::name());
            if (ignoreFieldsWithPrefix.size() > 0 && field_name.starts_with(ignoreFieldsWithPrefix)) {
                return;
            }
            const std::string newPrefix = prefix + field_name;

            // define ImGui actions for each primitive type
            // this list is not exhaustive. see https://en.cppreference.com/w/cpp/language/types
            // I should probably use concepts to group them anyway. ImGui doesn't have separate widgets for similar types like {float, double}

            // TODO custom component handlers for:
            // - std::vector
            // - std::unordered_map / std::map
            // - std::unordered_set / std::set
            // - entity/entityID (doing a drag & drop like unity would be cool. Also lookup by name would be nice)
            if constexpr (std::is_same_v<Vector2i, Dtype>) {
                if (ImGui::DragInt2(newPrefix.c_str(), &(f.value()->x), 1.0f, -INT_MAX, INT_MAX)) {
                    isChange = true;
                }

            } else if constexpr (std::is_same_v<Vector2f, Dtype>) {
                f32 dragSpeeds[] = {
                    ImGui::GetSlideSpeedLogarithmic(f.value()->x),
                    ImGui::GetSlideSpeedLogarithmic(f.value()->y),
                };
                const f32 minVal = -FLT_MAX;
                const f32 maxVal = FLT_MAX;
                if (ImGui::DragScalarNCustom(newPrefix.c_str(), ImGuiDataType_Float, 2, &(f.value()->x), dragSpeeds, &minVal, &maxVal, "%.2f",
                                             ImGuiSliderFlags_NoRoundToFormat)) {
                    isChange = true;
                }

            } else if constexpr (std::is_same_v<Color, Dtype>) {
                if (ImGui::ColorEdit4(newPrefix.c_str(), &(f.value()->r), ImGuiColorEditFlags_HDR)) {
                    isChange = true;
                }

            } else if constexpr (std::is_same_v<short, Dtype>) {
                print("unhandled integral type: ", type_of<Dtype>());

            } else if constexpr (std::is_same_v<int, Dtype>) {
                if (ImGui::DragInt(newPrefix.c_str(), f.value(), 1.0f, -INT_MAX, INT_MAX)) {
                    isChange = true;
                }

            } else if constexpr (std::is_same_v<unsigned int, Dtype>) {
                if (ImGui::InputScalar(newPrefix.c_str(), ImGuiDataType_U32, f.value())) {
                    isChange = true;
                }

            } else if constexpr (std::is_same_v<long, Dtype>) {
                print("unhandled integral type: ", type_of<Dtype>());

            } else if constexpr (std::is_same_v<long long, Dtype>) {
                print("unhandled integral type: ", type_of<Dtype>());

            } else if constexpr (std::is_same_v<float, Dtype>) {
                if (ImGui::DragFloat(newPrefix.c_str(), f.value(), ImGui::GetSlideSpeedLogarithmic(*f.value()), -FLT_MAX, +FLT_MAX, "%.2f",
                                     ImGuiSliderFlags_NoRoundToFormat)) {
                    isChange = true;
                }

            } else if constexpr (std::is_same_v<double, Dtype>) {
                print("unhandled integral type: ", type_of<Dtype>());

            } else if constexpr (std::is_same_v<bool, Dtype>) {
                if (ImGui::Checkbox(newPrefix.c_str(), f.value())) {
                    isChange = true;
                }

            } else if constexpr (std::is_same_v<char*, Dtype> || std::is_same_v<const char*, Dtype>) {
                ImGui::Text(*f.value());

            } else if constexpr (std::is_same_v<std::string, Dtype>) {
                if (ImGui::InputText(newPrefix.c_str(), f.value())) {
                    isChange = true;
                }

            } else if constexpr (std::is_pointer_v<Dtype>) {
                // do nothing

            } else if constexpr (std::is_enum_v<Dtype>) {
                // is a std::array<std::pair<std::string_view, Dtype>, N>
                constexpr auto enums = rfl::get_enumerator_array<Dtype>();
                std::array<const char*, enums.size()> enumNames;
                int selection = 0;
                for (size_t i = 0; i < enums.size(); i++) {
                    enumNames[i] = enums[i].first.data();
                    if (enums[i].second == *f.value()) {
                        selection = i;
                    }
                }
                if (ImGui::Combo(newPrefix.c_str(), &selection, enumNames.data(), enumNames.size())) {
                    *f.value() = enums[selection].second;
                    isChange = true;
                }

            } else if constexpr (std::is_integral_v<Dtype>) {
                // catch-all for primitive types that I am not handling
                print("unhandled integral type: ", type_of<Dtype>());

            } else {
                // Recursively render struct.
                // specify autoresize, otherwise the first child window will be huge
                ImGui::BeginChild(newPrefix.c_str(), ImVec2(0, 0), ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY);
                std::pair<Dtype, bool> result = imguiRenderStruct<Dtype>(*f.value(), newPrefix + "::", ignoreFieldsWithPrefix);
                if (result.second) {
                    *f.value() = result.first;
                    isChange = true;
                }
                ImGui::EndChild();
            }
        });
        return {thing, isChange};
    }
}

template <typename T>
static void componentEditor(ecs::Entity entity) {
    const std::string cmpName(type_of<T>());
    ImGui::BeginChild(cmpName.c_str(), ImVec2(0, 0), ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY);
    ImGui::TextColored(rlImGuiColors::Convert(rl::SKYBLUE), "%s", cmpName.c_str());
    std::pair<T, bool> updated = imguiRenderStruct<T>(entity.get<T>());
    if (updated.second) {
        if constexpr (std::is_same_v<T, Transform>) {
            // special setter
            entity.get<Transform>().set(updated.first, entity);
        } else {
            entity.set(updated.first);
        }
    }
    ImGui::EndChild();
}

static void printComponents(ecs::Entity entity, int xOffset = 0) {
    ImGui::PushID(entity.id());
    if (xOffset == 0) {
        xOffset = ImGui::GetCursorPosX();
    } else {
        ImGui::SetCursorPosX(xOffset);
    }
    ImGui::TextColored(rlImGuiColors::Convert(rl::ORANGE),
                       "%s Components:", entity.has<Name>() ? entity.get<Name>().name.c_str() : sprint("Entity ", entity.id()).c_str());
    ImGui::SetCursorPosX(xOffset);
    componentEditor<Transform>(entity);
    if (entity.has<Sprite>()) {
        componentEditor<Sprite>(entity);
    }
    if (entity.has<Name>()) {
        componentEditor<Name>(entity);
    }
    ImGui::Separator();
    entity.forChild(&printComponents, true, xOffset + 16);
    ImGui::PopID();
}

// needs to be separate, otherwise the graphical stuff in `draw` will be drawn under the imgui ui
void EditorUiSystem::draw() {
    ImGui::Begin("Inspector");
    for (ecs::Entity entity : mClickedEntities) {
        printComponents(entity);
    }
    ImGui::End();  // Inspector
}

}  // namespace whal

#endif

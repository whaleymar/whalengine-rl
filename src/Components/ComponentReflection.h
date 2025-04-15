#pragma once

#include <concepts>
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "TypeName.h"
#include "Util/ImguiUtil.h"
#include "Util/Print.h"
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include "rfl/enums.hpp"
#include "rfl/internal/has_reflection_type_v.hpp"

#include "Gfx/Color.h"
#include "Util/Vector.h"
#include "rfl/to_view.hpp"

namespace whal {

template <typename T>
concept IsCustomEditor = requires(T t) {
    { t.onEditorRender() } -> std::same_as<void>;
};

// Define a type alias to reduce verbosity
template <typename T>
using BaseType = std::remove_cvref_t<std::remove_pointer_t<T>>;

// anything where we can iterate like `for (auto& thing : collection)`
// vector<bool> excluded because we can't get a reference
template <typename T>
concept IsVectorLike =
    requires {
        typename BaseType<T>;  // Ensure the type can be stripped
    } &&
    (std::same_as<BaseType<T>, std::vector<typename T::value_type>> || std::same_as<BaseType<T>, std::deque<typename T::value_type>> ||
     std::same_as<BaseType<T>, std::forward_list<typename T::value_type>> || std::same_as<BaseType<T>, std::list<typename T::value_type>> ||
     std::same_as<BaseType<T>, std::set<typename T::value_type>> || std::same_as<BaseType<T>, std::multiset<typename T::value_type>> ||
     std::same_as<BaseType<T>, std::unordered_set<typename T::value_type>> ||
     std::same_as<BaseType<T>, std::unordered_multiset<typename T::value_type>>) &&
    !std::same_as<BaseType<T>, std::vector<bool>>;

template <typename T>
concept IsMapLike =
    requires {
        typename BaseType<T>;  // Ensure the type can be stripped
    } && (std::same_as<BaseType<T>, std::map<typename T::key_type, typename T::mapped_type>> ||
          std::same_as<BaseType<T>, std::multimap<typename T::key_type, typename T::mapped_type>> ||
          std::same_as<BaseType<T>, std::unordered_map<typename T::key_type, typename T::mapped_type>> ||
          std::same_as<BaseType<T>, std::unordered_multimap<typename T::key_type, typename T::mapped_type>>);

template <typename T>
concept IsStringLike = std::same_as<T, std::string> || std::same_as<T, char*> || std::same_as<T, const char*>;

// template <typename T>
// concept IsSmartPointer = std::same_as<T, std::unique_ptr<typename T::element_type>> || std::same_as<T, std::shared_ptr<typename T::element_type>>;

template <typename T>
concept IsReflectable = std::is_aggregate_v<T> || rfl::internal::has_reflection_type_v<T> || IsVectorLike<T> || IsMapLike<T> || IsStringLike<T>;

// Primitive == Anything which has specific ImGui rendering code
template <typename T>
concept IsPrimitive = std::is_fundamental_v<T> || std::same_as<T, Vector2i> || std::same_as<T, Vector2f> || std::same_as<T, Color> ||
                      std::is_pointer_v<T> || std::is_enum_v<T> || IsStringLike<T> || IsVectorLike<T> || IsMapLike<T>;

template <typename T>
    requires(!IsReflectable<T> && !IsPrimitive<T>)
static std::pair<T, bool> imguiRenderStruct(T thing, const std::string& prefix = "", const std::string& ignoreFieldsWithPrefix = "_") {
    static bool isLogged = false;
    if (!isLogged) {
        print("non-reflectable type in imguiRenderStruct: ", type_of<T>());
        isLogged = true;
    }
    return {thing, false};
}

template <typename Dtype>
    requires IsPrimitive<Dtype>
static void imguiRenderPrimitive(Dtype* thing, const std::string& newPrefix, const std::string& ignoreFieldsWithPrefix, bool& isChange);

template <typename T>
    requires IsPrimitive<T>
static std::pair<T, bool> imguiRenderStruct(T thing, const std::string& prefix = "", const std::string& ignoreFieldsWithPrefix = "_") {
    bool isChange = false;
    imguiRenderPrimitive(&thing, prefix, ignoreFieldsWithPrefix, isChange);
    return {thing, isChange};
}

// bool tracks if anything changed
template <typename T>
    requires(IsReflectable<T> && !IsPrimitive<T>)
static std::pair<T, bool> imguiRenderStruct(T thing, const std::string& prefix = "", const std::string& ignoreFieldsWithPrefix = "_") {
    if constexpr (rfl::internal::has_reflection_type_v<T>) {
        // make sure we use the reflectable type if it has one
        return imguiRenderStruct(thing.reflection(), prefix, ignoreFieldsWithPrefix);
    } else if constexpr (IsPrimitive<T>) {
        bool isChange = false;
        imguiRenderPrimitive(&thing, prefix, ignoreFieldsWithPrefix, isChange);
        return {thing, isChange};
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

            if constexpr (IsPrimitive<Dtype>) {
                imguiRenderPrimitive<Dtype>(f.value(), newPrefix, ignoreFieldsWithPrefix, isChange);
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

#ifndef NDEBUG

template <typename Dtype>
    requires IsPrimitive<Dtype>
static void imguiRenderPrimitive(Dtype* thing, const std::string& newPrefix, const std::string& ignoreFieldsWithPrefix, bool& isChange) {
    // define ImGui actions for each primitive type
    // this list is not exhaustive. see https://en.cppreference.com/w/cpp/language/types
    // I should probably use concepts to group them anyway. ImGui doesn't have separate widgets for similar types like {float, double}

    // RESEARCH custom component handlers for:
    // - IsMapLike
    // - entity/entityID (doing a drag & drop like unity would be cool. Also lookup by name would be nice)
    if constexpr (std::is_same_v<Vector2i, Dtype>) {
        if (ImGui::DragInt2(newPrefix.c_str(), &(thing->x), 1.0f, -INT_MAX, INT_MAX)) {
            isChange = true;
        }

    } else if constexpr (std::is_same_v<Vector2f, Dtype>) {
        f32 dragSpeeds[] = {
            ImGui::GetSlideSpeedLogarithmic(thing->x),
            ImGui::GetSlideSpeedLogarithmic(thing->y),
        };
        const f32 minVal = -FLT_MAX;
        const f32 maxVal = FLT_MAX;
        if (ImGui::DragScalarNCustom(newPrefix.c_str(), ImGuiDataType_Float, 2, &(thing->x), dragSpeeds, &minVal, &maxVal, "%.2f",
                                     ImGuiSliderFlags_NoRoundToFormat)) {
            isChange = true;
        }

    } else if constexpr (std::is_same_v<Color, Dtype>) {
        if (ImGui::ColorEdit4(newPrefix.c_str(), &(thing->r), ImGuiColorEditFlags_HDR)) {
            isChange = true;
        }

    } else if constexpr (std::is_same_v<short, Dtype>) {
        print("unhandled fundamental type: ", type_of<Dtype>());

    } else if constexpr (std::is_same_v<unsigned short, Dtype>) {
        // print("unhandled fundamental type: ", type_of<Dtype>());
        const u16 min = 0;
        const u16 max = 0xffff;
        if (ImGui::DragScalar(newPrefix.c_str(), ImGuiDataType_U16, thing, 1.0f, &min, &max)) {
            isChange = true;
        }

    } else if constexpr (std::is_same_v<int, Dtype>) {
        if (ImGui::DragInt(newPrefix.c_str(), thing, 1.0f, -INT_MAX, INT_MAX)) {
            isChange = true;
        }

    } else if constexpr (std::is_same_v<unsigned int, Dtype>) {
        if (ImGui::InputScalar(newPrefix.c_str(), ImGuiDataType_U32, thing)) {
            isChange = true;
        }

    } else if constexpr (std::is_same_v<long, Dtype>) {
        print("unhandled fundamental type: ", type_of<Dtype>());

    } else if constexpr (std::is_same_v<long long, Dtype>) {
        print("unhandled fundamental type: ", type_of<Dtype>());

    } else if constexpr (std::is_same_v<unsigned long, Dtype>) {
        if (ImGui::InputScalar(newPrefix.c_str(), ImGuiDataType_U64, thing)) {
            isChange = true;
        }

    } else if constexpr (std::is_same_v<float, Dtype>) {
        if (ImGui::DragFloat(newPrefix.c_str(), thing, ImGui::GetSlideSpeedLogarithmic(*thing), -FLT_MAX, +FLT_MAX, "%.2f",
                             ImGuiSliderFlags_NoRoundToFormat)) {
            isChange = true;
        }

    } else if constexpr (std::is_same_v<double, Dtype>) {
        print("unhandled fundamental type: ", type_of<Dtype>());

    } else if constexpr (std::is_same_v<bool, Dtype>) {
        if (ImGui::Checkbox(newPrefix.c_str(), thing)) {
            isChange = true;
        }

    } else if constexpr (std::is_same_v<char*, Dtype> || std::is_same_v<const char*, Dtype>) {
        ImGui::Text("%s", *thing);

    } else if constexpr (std::is_same_v<std::string, Dtype>) {
        if (ImGui::InputText(newPrefix.c_str(), thing)) {
            isChange = true;
        }
    } else if constexpr (IsVectorLike<Dtype>) {
        s32 i = 0;
        // RESEARCH handle unordered_ structs
        if (ImGui::TreeNode(newPrefix.c_str())) {
            for (auto& elem : *thing) {
                ImGui::PushID(i);
                auto result = imguiRenderStruct(elem, newPrefix + "::", ignoreFieldsWithPrefix);
                if (result.second) {
                    elem = result.first;
                    isChange = true;
                }
                ImGui::PopID();
                i++;
            }
            ImGui::TreePop();
        }

    } else if constexpr (IsMapLike<Dtype>) {
        print("unhandled type: MapLike");

    } else if constexpr (std::is_pointer_v<Dtype>) {
        // do nothing

    } else if constexpr (std::is_enum_v<Dtype>) {
        // is a std::array<std::pair<std::string_view, Dtype>, N>
        constexpr auto enums = rfl::get_enumerator_array<Dtype>();
        std::array<const char*, enums.size()> enumNames;
        int selection = 0;
        for (size_t i = 0; i < enums.size(); i++) {
            enumNames[i] = enums[i].first.data();
            if (enums[i].second == *thing) {
                selection = i;
            }
        }
        if (ImGui::Combo(newPrefix.c_str(), &selection, enumNames.data(), enumNames.size())) {
            *thing = enums[selection].second;
            isChange = true;
        }

    } else if constexpr (std::is_fundamental_v<Dtype>) {
        // catch-all for primitive types that I am not handling
        print("unhandled fundamental type: ", type_of<Dtype>());
    }
}

#endif

}  // namespace whal

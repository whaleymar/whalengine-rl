#ifndef NDEBUG
#include "ImguiUtil.h"
#include "MathUtil.h"
#include "imgui.h"
#include "imgui_internal.h"

namespace ImGui {

static const ImGuiDataTypeInfo S_DATA_TYPE_INFO[] = {
    {sizeof(char), "S8", "%d", "%d"},  // ImGuiDataType_S8
    {sizeof(unsigned char), "U8", "%u", "%u"},
    {sizeof(short), "S16", "%d", "%d"},  // ImGuiDataType_S16
    {sizeof(unsigned short), "U16", "%u", "%u"},
    {sizeof(int), "S32", "%d", "%d"},  // ImGuiDataType_S32
    {sizeof(unsigned int), "U32", "%u", "%u"},
#ifdef _MSC_VER
    {sizeof(ImS64), "S64", "%I64d", "%I64d"},  // ImGuiDataType_S64
    {sizeof(ImU64), "U64", "%I64u", "%I64u"},
#else
    {sizeof(ImS64), "S64", "%lld", "%lld"},  // ImGuiDataType_S64
    {sizeof(ImU64), "U64", "%llu", "%llu"},
#endif
    {sizeof(float), "float", "%.3f", "%f"},   // ImGuiDataType_Float (float are promoted to double in va_arg)
    {sizeof(double), "double", "%f", "%lf"},  // ImGuiDataType_Double
    {sizeof(bool), "bool", "%d", "%d"},       // ImGuiDataType_Bool
};

float GetSlideSpeedLogarithmic(float currentValue) {
    const f32 minSlideSpeed = 0.01;

    f32 slideSpeed = 0.025;
    const f32 absVal = math::abs(currentValue);
    if (absVal > 1.0f) {
        // log the value so speed isn't crazy for large values
        slideSpeed = slideSpeed * std::log(absVal) * 4.0f;
    } else {
        slideSpeed = slideSpeed * absVal;
    }
    if (slideSpeed < minSlideSpeed) {
        slideSpeed = minSlideSpeed;
    }
    return slideSpeed;
}

bool DragScalarNCustom(const char* label, ImGuiDataType data_type, int count, void* p_data, float* v_speeds, const void* p_min, const void* p_max,
                       const char* format, ImGuiSliderFlags flags) {
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    bool value_changed = false;
    BeginGroup();
    PushID(label);
    PushMultiItemsWidths(count, CalcItemWidth());
    size_t type_size = S_DATA_TYPE_INFO[data_type].Size;
    for (int i = 0; i < count; i++) {
        PushID(i);
        if (i > 0)
            SameLine(0, g.Style.ItemInnerSpacing.x);
        value_changed |= DragScalar("", data_type, p_data, v_speeds[i], p_min, p_max, format, flags);
        PopID();
        PopItemWidth();
        p_data = (void*)((char*)p_data + type_size);
    }
    PopID();

    const char* label_end = FindRenderedTextEnd(label);
    if (label != label_end) {
        SameLine(0, g.Style.ItemInnerSpacing.x);
        TextEx(label, label_end);
    }

    EndGroup();
    return value_changed;
}

}  // namespace ImGui
#endif

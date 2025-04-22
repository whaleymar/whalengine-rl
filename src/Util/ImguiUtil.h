#pragma once

#ifndef NDEBUG

typedef int ImGuiDataType;     // -> enum ImGuiDataType_        // Enum: A primary data type
typedef int ImGuiSliderFlags;  // -> enum ImGuiSliderFlags_     // Flags: for DragFloat(), DragInt(), SliderFloat(), SliderInt() etc.

namespace ImGui {

// Scales slide speed with the magnitude so it's easier to adjust small values
// ImGui has a flag for this but it seems super broken
float GetSlideSpeedLogarithmic(float currentValue);

// supports per-scalar slide speeds
bool DragScalarNCustom(const char* label, ImGuiDataType data_type, int count, void* p_data, float* v_speeds, const void* p_min, const void* p_max,
                       const char* format, ImGuiSliderFlags flags);

}  // namespace ImGui
#endif

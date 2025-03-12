#pragma once

#ifndef NDEBUG
#include <unordered_set>

namespace whal {

class IRenderDebug;
class DebugRenderMgr {
public:
    static DebugRenderMgr& instance() {
        static DebugRenderMgr instance_;
        return instance_;
    }

    static void add(IRenderDebug* obj) { instance().mObjs.insert(obj); }
    static void remove(IRenderDebug* obj) { instance().mObjs.erase(obj); };
    static void drawEditor();
    static void drawDebug();

private:
    std::unordered_set<IRenderDebug*> mObjs;
};

// make sure an empty class definition exists for inheritance reasons
class IRenderDebug {
public:
    virtual ~IRenderDebug();
    virtual void drawDebug() {}
    virtual void drawEditor() {}

protected:
    IRenderDebug();
};

}  // namespace whal

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

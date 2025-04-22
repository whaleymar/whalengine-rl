#include "IRender.h"
#include "Sys/Renderer.h"
#include "Sys/System.h"

namespace whal {

IRender::IRender() {
    Graphics.registerRenderer(this);
}

IRender::~IRender() {
    Graphics.unregisterRenderer(this);
}

IRenderLight::IRenderLight() {
    Graphics.registerLightRenderer(this);
}

IRenderLight::~IRenderLight() {
    Graphics.unregisterLightRenderer(this);
}

#ifndef NDEBUG
IRenderDebug::IRenderDebug() {
    Graphics.registerDebugRenderer(this);
}

IRenderDebug::~IRenderDebug() {
    Graphics.unregisterDebugRenderer(this);
}
#endif

}  // namespace whal

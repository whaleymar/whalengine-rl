#pragma once

#include "Util/Types.h"

namespace whal {
class IGame;
}

extern "C" {
bool _EngineStart();
void _EngineSetGame(whal::IGame& game);
bool _EngineIsValid();
void _EngineReset();
void _EngineSleep(float seconds);
bool _EngineIsQuit();
void _EngineUpdate();
void _EngineEnd();
bool _EngineIsHotReloadRequested();

s32 _EngineGetWindowWidth();
s32 _EngineGetWindowHeight();
const char* _EngineGetWindowTitle();
const char* _EngineGetIconPath();
s32 _EngineGetTargetFPS();

#ifndef NDEBUG
bool _EngineIsEditorMode();
void _EngineSetEditorMode(bool);
bool _EngineIsEditorSuspend();
void _EngineSetEditorSuspend(bool);
#endif

#ifdef __EMSCRIPTEN__
typedef struct EmscriptenVisibilityChangeEvent EmscriptenVisibilityChangeEvent;
bool _EngineVisibilityChangeCallback(int eventType, const EmscriptenVisibilityChangeEvent* event, void* userData);
#endif
}

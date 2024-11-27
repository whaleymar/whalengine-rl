#pragma once

#include "Util/Types.h"

/////////////////////////////////////////////////////////////
////////////////////// BUILD OPTIONS ////////////////////////
/////////////////////////////////////////////////////////////

#ifndef __EMSCRIPTEN__
#define USE_THREADS
#endif

/////////////////////////////////////////////////////////////
///////////////////////// WHALECS ///////////////////////////
/////////////////////////////////////////////////////////////

#define MAX_ENTITIES 5000
#define MAX_COMPONENTS 64

/////////////////////////////////////////////////////////////
///////////////// DEBUG SETTINGS ////////////////////////////
/////////////////////////////////////////////////////////////

extern bool EDITOR_MODE;

/////////////////////////////////////////////////////////////
///////////////// RAYLIB GRAPHICS ///////////////////////////
/////////////////////////////////////////////////////////////

extern const char* WINDOW_TITLE;
extern s32 FPS_TARGET;

// stuff that doesn't change (for now)
// inline constexpr s32 FPS_TARGET = 60;
inline constexpr s32 PIXELS_PER_TILE = 8;
inline constexpr f32 FPIXELS_PER_TILE = static_cast<f32>(PIXELS_PER_TILE);

// 3 window sizes I use:
// Render: The actual window size on your screen
// Pixels: Window size that the game uses

// TODO should have a resizable window that triggers some event

extern s32 WINDOW_WIDTH_RENDER;
extern s32 WINDOW_HEIGHT_RENDER;
extern s32 WINDOW_WIDTH_GAME;
extern s32 WINDOW_HEIGHT_GAME;

extern "C" {
s32 WhalGetRenderWidth();
s32 WhalGetRenderHeight();
const char* WhalGetWindowTitle();
s32 WhalGetTargetFPS();
bool WhalIsEditorMode();
void WhalSetEditorMode(bool);
}

// DERIVED STUFF
extern f32 FWINDOW_WIDTH_RENDER;
extern f32 FWINDOW_HEIGHT_RENDER;
extern f32 FWINDOW_WIDTH_GAME;
extern f32 FWINDOW_HEIGHT_GAME;
extern f32 VIRTUAL_SCREEN_RATIO;

/////////////////////////////////////////////////////////////
////////////////////// FILE PATHS ///////////////////////////
/////////////////////////////////////////////////////////////

inline const char* DATA_DIR = "data";
inline const char* FONT_PATH = "data/other-font.ttf";
inline const char* SPRITE_TEXTURE_PATH = "data/sprite/atlas0.png";
inline const char* PALETTE_TEXTURE_PATH = "data/texture/palette.png";
inline const char* ATLAS_METADATA_PATH = "data/sprite/atlas.xml";
inline const char* TILED_PROJECT_FILE = "project.tiled-project";  // path relative to map dir
inline const char* ICON_IMAGE_PATH = "data/icon.png";

/////////////////////////////////////////////////////////////
////////////////////// GAME SETTINGS ////////////////////////
/////////////////////////////////////////////////////////////

enum class WorldType2D { TopDown, SideScroller };
inline constexpr WorldType2D WORLD_TYPE = WorldType2D::TopDown;

// This defines how an object's "Floating" parameter affects its screen position.
// For example, if an object is floating 8 units in the air, then a mult of 0.5 means it's drawn 4px higher.
inline constexpr f32 FLOAT_HEIGHT_MULT = 0.5;

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
///////////////// RAYLIB GRAPHICS ///////////////////////////
/////////////////////////////////////////////////////////////

inline const char* WINDOW_TITLE = "whalengine";

inline constexpr s32 FPS_TARGET = 60;

// 3 window sizes I use:
// Render: The actual window size on your screen
// Pixels: Window size that the game uses

// TODO should have a resizable window that triggers some event

inline constexpr s32 WINDOW_WIDTH_RENDER = 1280;
inline constexpr s32 WINDOW_HEIGHT_RENDER = 720;

inline constexpr s32 WINDOW_WIDTH_GAME = 320;
inline constexpr s32 WINDOW_HEIGHT_GAME = 180;

inline constexpr s32 PIXELS_PER_TILE = 8;

// DERIVED STUFF
inline constexpr f32 FWINDOW_WIDTH_RENDER = WINDOW_WIDTH_RENDER;
inline constexpr f32 FWINDOW_HEIGHT_RENDER = WINDOW_HEIGHT_RENDER;

inline constexpr f32 FWINDOW_WIDTH_GAME = WINDOW_WIDTH_GAME;
inline constexpr f32 FWINDOW_HEIGHT_GAME = WINDOW_HEIGHT_GAME;

inline constexpr f32 VIRTUAL_SCREEN_RATIO = FWINDOW_WIDTH_RENDER / FWINDOW_WIDTH_GAME;

inline constexpr f32 FPIXELS_PER_TILE = static_cast<f32>(PIXELS_PER_TILE);

/////////////////////////////////////////////////////////////
////////////////////// FILE PATHS ///////////////////////////
/////////////////////////////////////////////////////////////

inline const char* DATA_DIR = "src/Game/data";
inline const char* FONT_PATH = "src/Game/data/other-font.ttf";
inline const char* SPRITE_TEXTURE_PATH = "src/Game/data/sprite/atlas0.png";
inline const char* PALETTE_TEXTURE_PATH = "src/Game/data/texture/palette.png";
inline const char* ATLAS_METADATA_PATH = "src/Game/data/sprite/atlas.xml";
inline const char* TILED_PROJECT_FILE = "project.tiled-project";  // path relative to map dir
inline const char* ICON_IMAGE_PATH = "src/Game/data/icon.png";

/////////////////////////////////////////////////////////////
////////////////////// GAME SETTINGS ////////////////////////
/////////////////////////////////////////////////////////////

enum class WorldType2D { TopDown, SideScroller };
inline constexpr WorldType2D WORLD_TYPE = WorldType2D::TopDown;

// This defines how an object's "Floating" parameter affects its screen position.
// For example, if an object is floating 8 units in the air, then a mult of 0.5 means it's drawn 4px higher.
inline constexpr f32 FLOAT_HEIGHT_MULT = 0.5;

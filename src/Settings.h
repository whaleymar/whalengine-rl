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
// Actual: The actual window size on a computer
// Pixels: Window size that OpenGL uses
// Texels: Window size in in-game texel units

// TODO should have a resizable window that triggers some event

inline constexpr s32 WINDOW_WIDTH_ACTUAL = 1280;
inline constexpr s32 WINDOW_HEIGHT_ACTUAL = 720;

inline constexpr s32 WINDOW_WIDTH_PIXELS = 320;
inline constexpr s32 WINDOW_HEIGHT_PIXELS = 180;
inline constexpr s32 PIXELS_PER_TEXEL = 1;  // TODO deprecate this and the float version

inline constexpr s32 TEXELS_PER_TILE = 8;

// DERIVED STUFF
inline constexpr f32 FWINDOW_WIDTH_ACTUAL = WINDOW_WIDTH_ACTUAL;
inline constexpr f32 FWINDOW_HEIGHT_ACTUAL = WINDOW_HEIGHT_ACTUAL;

inline constexpr f32 FWINDOW_WIDTH_PIXELS = WINDOW_WIDTH_PIXELS;
inline constexpr f32 FWINDOW_HEIGHT_PIXELS = WINDOW_HEIGHT_PIXELS;

inline constexpr f32 VIRTUAL_SCREEN_RATIO = FWINDOW_WIDTH_ACTUAL / FWINDOW_WIDTH_PIXELS;

inline constexpr f32 FPIXELS_PER_TEXEL = PIXELS_PER_TEXEL;
inline constexpr f32 FTEXELS_PER_TILE = TEXELS_PER_TILE;
inline constexpr f32 FTEXELS_PER_PIXEL = 1 / FPIXELS_PER_TEXEL;
inline constexpr s32 PIXELS_PER_TILE = PIXELS_PER_TEXEL * TEXELS_PER_TILE;
inline constexpr f32 FPIXELS_PER_TILE = FPIXELS_PER_TEXEL * FTEXELS_PER_TILE;

inline constexpr s32 WINDOW_WIDTH_TEXELS = WINDOW_WIDTH_PIXELS / PIXELS_PER_TEXEL;
inline constexpr f32 FWINDOW_WIDTH_TEXELS = FWINDOW_WIDTH_PIXELS / FPIXELS_PER_TEXEL;
inline constexpr s32 WINDOW_HEIGHT_TEXELS = WINDOW_HEIGHT_PIXELS / PIXELS_PER_TEXEL;
inline constexpr f32 FWINDOW_HEIGHT_TEXELS = FWINDOW_HEIGHT_PIXELS / FPIXELS_PER_TEXEL;

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

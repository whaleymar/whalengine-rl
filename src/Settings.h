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

#ifndef NDEBUG
extern bool EDITOR_MODE;
extern bool EDITOR_SUSPEND;        // pauses everything except rendering
extern bool EDITOR_FRAME_ADVANCE;  // pauses everything in System::update except for calling Game::update
extern bool EDITOR_FRAME_DO_NEXT;
extern bool VIEW_COLLIDERS_MODE;
#endif

/////////////////////////////////////////////////////////////
///////////////// RAYLIB GRAPHICS ///////////////////////////
/////////////////////////////////////////////////////////////

extern const char* WINDOW_TITLE;
extern s32 FPS_TARGET;

// stuff that doesn't change (for now)
inline constexpr s32 PIXELS_PER_TILE = 8;
inline constexpr f32 FPIXELS_PER_TILE = static_cast<f32>(PIXELS_PER_TILE);

// several window sizes I use:
// OS: The size of the Operating System window the game is running inside of.
// RENDER: The size that the game is drawn in. This may be the same as Screen size, or different if you want to have a consistent aspect ratio when
//         the window is resized / The game is running within the engine editor (debug only).
// GAME: Window size that the game uses (aka the Native resolution). One pixel here equals 1 world unit.
// DOCK (debug): Size of the imgui window that the game is docked in.
// DOCK (release): Same as OS
// STRETCH: The size that the drawn game is scaled to. If the render window is uncapped, this is the same as the render size.

enum class ScreenResolution {
    OS,
    Render,
    Game,
    Dock,
    Stretched,
};

extern s32 WINDOW_WIDTH_OS;
extern s32 WINDOW_HEIGHT_OS;
extern s32 WINDOW_WIDTH_RENDER;
extern s32 WINDOW_HEIGHT_RENDER;
extern s32 WINDOW_WIDTH_GAME;
extern s32 WINDOW_HEIGHT_GAME;
extern s32 WINDOW_WIDTH_DOCK;
extern s32 WINDOW_HEIGHT_DOCK;
extern s32 WINDOW_WIDTH_STRETCH;
extern s32 WINDOW_HEIGHT_STRETCH;

// The Render size can optionally be capped. Either to preserve pixel-perfect drawing or to ensure
// expensive shader effects are run at a lower resolution.
extern s32 WINDOW_MAX_WIDTH_RENDER;
extern s32 WINDOW_MAX_HEIGHT_RENDER;
extern bool IS_CAP_RENDER_WINDOW;

// If the OS and Render dimensions are not the same, these variables describe the render window's position (top left) on the screen
extern s32 WINDOW_POS_OS_X;  // Render window's X position on the OS window
extern s32 WINDOW_POS_OS_Y;  // Render window's Y position on the OS window

// DERIVED STUFF
extern f32 FWINDOW_WIDTH_OS;
extern f32 FWINDOW_HEIGHT_OS;
extern f32 FWINDOW_WIDTH_RENDER;
extern f32 FWINDOW_HEIGHT_RENDER;
extern f32 FWINDOW_WIDTH_GAME;
extern f32 FWINDOW_HEIGHT_GAME;
extern f32 FWINDOW_WIDTH_DOCK;
extern f32 FWINDOW_HEIGHT_DOCK;
extern f32 FWINDOW_WIDTH_STRETCH;
extern f32 FWINDOW_HEIGHT_STRETCH;
extern f32 VIRTUAL_SCREEN_RATIO;
extern f32 VIRTUAL_SCREEN_RATIO_STRETCH;

/////////////////////////////////////////////////////////////
////////////////////// FILE PATHS ///////////////////////////
/////////////////////////////////////////////////////////////

extern const char* DATA_DIR;
extern const char* FONT_PATH;
extern const char* SPRITE_TEXTURE_PATH;
extern const char* PALETTE_TEXTURE_PATH;
extern const char* NOISE_TEXTURE_PATH;
extern const char* ATLAS_METADATA_PATH;
extern const char* TILED_PROJECT_FILE;
extern const char* ICON_IMAGE_PATH;
extern const char* SHADER_DIR;

/////////////////////////////////////////////////////////////
////////////////////// GAME SETTINGS ////////////////////////
/////////////////////////////////////////////////////////////

enum class WorldType2D { TopDown, SideScroller };
// games can define WHAL_SIDESCROLLER (e.g. with a compile definition) to use side-scroller physics
#ifdef WHAL_SIDESCROLLER
inline constexpr WorldType2D WORLD_TYPE = WorldType2D::SideScroller;
#else
inline constexpr WorldType2D WORLD_TYPE = WorldType2D::TopDown;
#endif

// This defines how an object's "Floating" parameter affects its screen position.
// For example, if an object is floating 8 units in the air, then a mult of 0.5 means it's drawn 4px higher.
inline constexpr f32 FLOAT_HEIGHT_MULT = 0.5;

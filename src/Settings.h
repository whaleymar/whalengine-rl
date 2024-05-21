#pragma once

#include "Util/Types.h"

namespace {

/////////////////////////////////////////////////////////////
///////////////// RAYLIB GRAPHICS ///////////////////////////
/////////////////////////////////////////////////////////////

const char* WINDOW_TITLE = "whalengine";

constexpr s32 FPS_TARGET = 60;

// 3 window sizes I use:
// Actual: The actual window size on a computer
// Pixels: Window size that OpenGL uses
// Texels: Window size in in-game texel units

constexpr s32 WINDOW_WIDTH_ACTUAL = 1280;
constexpr s32 WINDOW_HEIGHT_ACTUAL = 720;

constexpr s32 WINDOW_WIDTH_PIXELS = 640;
constexpr s32 WINDOW_HEIGHT_PIXELS = 360;

constexpr s32 PIXELS_PER_TEXEL = 2;
constexpr s32 TEXELS_PER_TILE = 8;

// DERIVED STUFF
constexpr f32 FWINDOW_WIDTH_ACTUAL = WINDOW_WIDTH_ACTUAL;
constexpr f32 FWINDOW_HEIGHT_ACTUAL = WINDOW_HEIGHT_ACTUAL;

constexpr f32 FWINDOW_WIDTH_PIXELS = WINDOW_WIDTH_PIXELS;
constexpr f32 FWINDOW_HEIGHT_PIXELS = WINDOW_HEIGHT_PIXELS;

constexpr f32 FPIXELS_PER_TEXEL = PIXELS_PER_TEXEL;
constexpr f32 FTEXELS_PER_TILE = TEXELS_PER_TILE;
constexpr f32 FTEXELS_PER_PIXEL = 1 / PIXELS_PER_TEXEL;
constexpr s32 PIXELS_PER_TILE = PIXELS_PER_TEXEL * TEXELS_PER_TILE;
constexpr f32 FPIXELS_PER_TILE = FPIXELS_PER_TEXEL * FTEXELS_PER_TILE;

constexpr s32 WINDOW_WIDTH_TEXELS = WINDOW_WIDTH_PIXELS / PIXELS_PER_TEXEL;
constexpr s32 WINDOW_HEIGHT_TEXELS = WINDOW_HEIGHT_PIXELS / PIXELS_PER_TEXEL;

/////////////////////////////////////////////////////////////
////////////////////// FILE PATHS ///////////////////////////
/////////////////////////////////////////////////////////////

const char* SPRITE_TEXTURE_PATH = "data/sprite/atlas0.png";
const char* ATLAS_METADATA_PATH = "data/sprite/atlas.xml";
const char* TILED_PROJECT_FILE = "project.tiled-project";

}  // namespace

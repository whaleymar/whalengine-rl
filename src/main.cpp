#include "raylib.h"

#include "Settings.h"
#include "Types.h"

int main(void) {
    InitWindow(WINDOW_WIDTH_ACTUAL, WINDOW_HEIGHT_ACTUAL, "whalengine");

    // do this before any font/texture stuff or the settings seem to get fucked
    Camera2D camera;
    camera.target = Vector2(0.0f, 0.0f);
    camera.offset = Vector2(WINDOW_WIDTH_ACTUAL / 2.0f, WINDOW_HEIGHT_ACTUAL / 2.0f);
    camera.zoom = 1.0f;

    Font testFont = LoadFontEx("data/other-font.ttf", 18, 0, 0);

    Rectangle testRect = {400, 280, 40, 40};
    camera.target = {testRect.x + 20, testRect.y + 20};

    Texture2D testSprite = LoadTexture("data/sprite/atlas0.png");
    // f32 texWidth = static_cast<f32>(testSprite.width);
    // f32 texHeight = static_cast<f32>(testSprite.height);
    // source retangle: part of texture to use for drawing
    // Rectangle sourceRect = {0.0f, 0.0f, texWidth, texHeight};
    Rectangle sourceRect = {675, 0, 16, 16};

    // destination rectangle: screen rect we draw to
    // Rectangle destRect = {WINDOW_WIDTH_ACTUAL / 2.0f, WINDOW_HEIGHT_ACTUAL / 2.0f, texWidth, texHeight};
    Rectangle destRect = {WINDOW_WIDTH_ACTUAL / 2.0f, WINDOW_HEIGHT_ACTUAL / 2.0f, 64, 64};  // scaling 4x
    // origin is the reference point used for rotation and scaling
    // relative to the DESTINATION rectangle size
    Vector2 origin = {32, 32};
    s32 rotation = 0;

    SetTargetFPS(FPS_TARGET);

    while (!WindowShouldClose()) {
        rotation++;
        BeginDrawing();

        ClearBackground(RAYWHITE);
        // DrawText("You expressed expressedly", 190, 200, 20, LIGHTGRAY);
        DrawTextEx(testFont, "You expressed expressedly", {190, 200}, 20, 0, LIGHTGRAY);

        BeginMode2D(camera);

        DrawRectangle(-6000, 320, 13000, 8000, DARKGRAY);
        DrawRectangleRec(testRect, RED);
        DrawTexturePro(testSprite, sourceRect, destRect, origin, rotation, WHITE);

        EndMode2D();

        EndDrawing();
    }

    CloseWindow();

    return 0;
}

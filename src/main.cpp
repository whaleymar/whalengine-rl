// #include "raylib.h"
//
// #include "Settings.h"
// #include "Types.h"
//
// int main(void) {
//     InitWindow(WINDOW_WIDTH_ACTUAL, WINDOW_HEIGHT_ACTUAL, "whalengine");
//
//     // do this before any font/texture stuff or the settings seem to get fucked
//     Camera2D camera;
//     camera.target = Vector2(0.0f, 0.0f);
//     camera.offset = Vector2(WINDOW_WIDTH_ACTUAL / 2.0f, WINDOW_HEIGHT_ACTUAL / 2.0f);
//     camera.zoom = 1.0f;
//
//     Font testFont = LoadFontEx("data/other-font.ttf", 18, 0, 0);
//
//     Rectangle testRect = {400, 280, 40, 40};
//     camera.target = {testRect.x + 20, testRect.y + 20};
//
//     Texture2D testSprite = LoadTexture("data/sprite/atlas0.png");
//     // f32 texWidth = static_cast<f32>(testSprite.width);
//     // f32 texHeight = static_cast<f32>(testSprite.height);
//     // source retangle: part of texture to use for drawing
//     // Rectangle sourceRect = {0.0f, 0.0f, texWidth, texHeight};
//     Rectangle sourceRect = {675, 0, 16, 16};
//
//     // destination rectangle: screen rect we draw to
//     // Rectangle destRect = {WINDOW_WIDTH_ACTUAL / 2.0f, WINDOW_HEIGHT_ACTUAL / 2.0f, texWidth, texHeight};
//     Rectangle destRect = {WINDOW_WIDTH_ACTUAL / 2.0f, WINDOW_HEIGHT_ACTUAL / 2.0f, 64, 64};  // scaling 4x
//     // origin is the reference point used for rotation and scaling
//     // relative to the DESTINATION rectangle size
//     // Vector2 origin = {32, 32};
//     Vector2 origin = {0, 0};
//     s32 rotation = 0;
//
//     SetTargetFPS(FPS_TARGET);
//
//     while (!WindowShouldClose()) {
//         // rotation++;
//         BeginDrawing();
//
//         ClearBackground(RAYWHITE);
//         // DrawText("You expressed expressedly", 190, 200, 20, LIGHTGRAY);
//         DrawTextEx(testFont, "You expressed expressedly", {190, 200}, 20, 0, LIGHTGRAY);
//
//         BeginMode2D(camera);
//
//         DrawRectangle(-6000, 320, 13000, 8000, DARKGRAY);
//         DrawRectangleRec(testRect, RED);
//         // DrawTexturePro(testSprite, sourceRect, destRect, origin, rotation, WHITE);
//         DrawTexturePro(testSprite, sourceRect, testRect, origin, rotation, WHITE);
//
//         EndMode2D();
//
//         EndDrawing();
//     }
//
//     CloseWindow();
//
//     return 0;
// }

#include <fmod.hpp>
#include "fmod_common.h"
#include "raylib.h"

#include <math.h>  // Required for: sinf(), cosf()

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void) {
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    const int virtualScreenWidth = 160;
    const int virtualScreenHeight = 90;

    const float virtualRatio = (float)screenWidth / (float)virtualScreenWidth;

    InitWindow(screenWidth, screenHeight, "raylib [core] example - smooth pixel-perfect camera");

    Camera2D worldSpaceCamera = {0};  // Game world camera
    worldSpaceCamera.zoom = 1.0f;

    Camera2D screenSpaceCamera = {0};  // Smoothing camera
    screenSpaceCamera.zoom = 1.0f;

    RenderTexture2D target = LoadRenderTexture(virtualScreenWidth, virtualScreenHeight);  // This is where we'll draw all our objects.

    Rectangle rec01 = {250.0f, 150.0f, 200.0f, 200.0f};
    // Rectangle rec02 = {90.0f, 55.0f, 30.0f, 10.0f};
    // Rectangle rec03 = {80.0f, 65.0f, 15.0f, 25.0f};
    // Rectangle rec01 = {70.0f, 35.0f, 20.0f, 20.0f};
    // Rectangle rec02 = {90.0f, 55.0f, 30.0f, 10.0f};
    // Rectangle rec03 = {80.0f, 65.0f, 15.0f, 25.0f};

    // The target's height is flipped (in the source Rectangle), due to OpenGL reasons
    Rectangle sourceRec = {0.0f, 0.0f, (float)target.texture.width, -(float)target.texture.height};
    Rectangle destRec = {-virtualRatio, -virtualRatio, screenWidth + (virtualRatio * 2), screenHeight + (virtualRatio * 2)};

    Vector2 origin = {0.0f, 0.0f};

    float rotation = 0.0f;

    float cameraX = 0.0f;
    float cameraY = 0.0f;

    // AUDIO SHIT
    // ------------------------------------------

    FMOD::System* system = nullptr;
    FMOD::System_Create(&system);

    system->init(512, FMOD_INIT_NORMAL, nullptr);

    FMOD::Sound* sound = nullptr;
    system->createStream("data/audio/music/provingGroundsTheme.mp3", FMOD_DEFAULT, nullptr, &sound);

    FMOD::Channel* channel = nullptr;
    system->playSound(sound, nullptr, false, &channel);
    bool isPlayingAudio = true;

    // ------------------------------------------

    SetTargetFPS(60);
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())  // Detect window close button or ESC key
    {
        // AUDIO SHIT JODSHFO{ISAJDFJSDJF:ILJ:DSFL
        if (isPlayingAudio) {
            system->update();
            channel->isPlaying(&isPlayingAudio);
        }

        // Update
        //----------------------------------------------------------------------------------
        // rotation += 60.0f * GetFrameTime();  // Rotate the rectangles, 60 degrees per second

        // Make the camera move to demonstrate the effect
        cameraX = (sinf(GetTime()) * 500.0f) - 90.0f;
        // cameraY = cosf(GetTime()) * 30.0f;

        // Set the camera's target to the values computed above
        screenSpaceCamera.target = (Vector2){cameraX, cameraY};

        // Round worldSpace coordinates, keep decimals into screenSpace coordinates
        worldSpaceCamera.target.x = (int)screenSpaceCamera.target.x;
        screenSpaceCamera.target.x -= worldSpaceCamera.target.x;
        screenSpaceCamera.target.x *= virtualRatio;

        worldSpaceCamera.target.y = (int)screenSpaceCamera.target.y;
        screenSpaceCamera.target.y -= worldSpaceCamera.target.y;
        screenSpaceCamera.target.y *= virtualRatio;
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();  // NEW
        // BeginTextureMode(target);
        ClearBackground(RAYWHITE);

        BeginMode2D(worldSpaceCamera);
        // BeginMode2D(screenSpaceCamera);
        DrawRectanglePro(rec01, origin, rotation, BLACK);
        // DrawRectanglePro(rec02, origin, -rotation, RED);
        // DrawRectanglePro(rec03, origin, rotation + 45.0f, BLUE);
        // EndMode2D();
        // EndTextureMode();

        // BeginDrawing();
        // ClearBackground(RED);

        // BeginMode2D(screenSpaceCamera);
        // DrawTexturePro(target.texture, sourceRec, destRec, origin, 0.0f, WHITE);
        EndMode2D();

        // DrawText(TextFormat("Screen resolution: %ix%i", screenWidth, screenHeight), 10, 10, 20, DARKBLUE);
        // DrawText(TextFormat("World resolution: %ix%i", virtualScreenWidth, virtualScreenHeight), 10, 40, 20, DARKGREEN);
        // DrawFPS(GetScreenWidth() - 95, 10);
        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // AUDIO SHIT

    sound->release();
    system->close();
    system->release();

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadRenderTexture(target);  // Unload render texture

    CloseWindow();  // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

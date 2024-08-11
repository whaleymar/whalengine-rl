#include "PauseMenu.h"
#include <raylib.h>

#include "Events/Events.h"
#include "Game.h"
#include "Settings.h"
#include "System.h"
#include "Util/Print.h"

namespace whal {

#ifndef NDEBUG
static const char* S_BUTTON_TO_NAME[] = {"Resume", "Restart", "Quick Restart", "Fullscreen: {}", "Exit"};
#else
static const char* S_BUTTON_TO_NAME[] = {"Resume", "Restart", "Fullscreen: {}", "Exit"};
#endif

s32 pythonMod(s32 a, s32 b) {
    // behaves like a % b in python (different from c++ for negative #s)
    return (b + (a % b)) % b;
}

void PauseMenu::onEvent(whal::ButtonPressEvent, InputType input) {
    if (!isActive()) {
        if (input == InputType::PAUSE) {
            activate();
            System::audio.playMenuClip(Sfx::MENU_OPEN, 0.33);
        }
        return;
    }

    switch (input) {
    case InputType::UP:
        mCursorOption = static_cast<Button>(pythonMod(mCursorOption - 1, N_BUTTONS));
        System::audio.playMenuClip(Sfx::MENU_MOVE, 0.33);
        break;
    case InputType::DOWN:
        mCursorOption = static_cast<Button>(pythonMod(mCursorOption + 1, N_BUTTONS));
        System::audio.playMenuClip(Sfx::MENU_MOVE, 0.33);
        break;
    case InputType::PAUSE:
        deactivate();
        System::audio.playMenuClip(Sfx::MENU_CLOSE, 0.33);
        break;
    case InputType::LEFT:
    case InputType::RIGHT:
        if (mCursorOption == Button::FullScreenToggle) {
            doCursorAction();
            System::audio.playMenuClip(Sfx::MENU_SELECT, 0.33);
        }
        break;
    case InputType::OK:
        doCursorAction();
        System::audio.playMenuClip(Sfx::MENU_SELECT, 0.33);
        break;
    default:
        break;
    }
}

void PauseMenu::draw(Font& font) const {
    if (!isActive()) {
        return;
    }
    constexpr s32 spacing = 24;                                 // PARAM
    constexpr s32 fontSize = 48 * VIRTUAL_SCREEN_RATIO / 4.0f;  // PARAM
    constexpr s32 spacingX = 0;                                 // PARAM

    constexpr s32 lineheight = spacing + fontSize;
    constexpr s32 menuHeight = lineheight * N_BUTTONS - spacing;  // n-1 fence posts
    constexpr s32 startHeight = WINDOW_HEIGHT_ACTUAL / 2 - menuHeight / 2;

    for (s32 i = 0; i < N_BUTTONS; i++) {
        std::string fullScreenText = whal_format(S_BUTTON_TO_NAME[i], IsWindowFullscreen() ? "On" : "Off");
        const char* buttonText = i != static_cast<s32>(FullScreenToggle) ? S_BUTTON_TO_NAME[i] : fullScreenText.c_str();
        Color color = static_cast<Button>(i) == mCursorOption ? RED : WHITE;
        Vector2 textDimensions = MeasureTextEx(font, buttonText, fontSize, spacingX);
        DrawTextEx(font, buttonText, Vector2(WINDOW_WIDTH_ACTUAL / 2 - textDimensions.x / 2, startHeight + lineheight * i), fontSize, spacingX,
                   color);
    }
}

void PauseMenu::doCursorAction() {
    switch (mCursorOption) {
    case Button::Resume:
        deactivate();
        break;
    case Button::Restart: {
        deactivate();
        Game::instance().reloadScene(true);
        break;
    }
    case Button::FullScreenToggle: {
        ToggleFullscreen();
        break;
    }
    case Button::Exit:
        System::quit();
        break;
#ifndef NDEBUG
    case Button::QuickRestart: {
        deactivate();
        Game::instance().reloadScene(false);
        break;
    }
#endif  // !NDEBUG
    default:
        print("Error: unhandled button");
    }
}

void PauseMenu::activate() {
    if (mIsActive) {
        return;
    }
    System::setPaused(true);
    mPrevMusicFilter = System::audio.getFilterMusic();
    System::audio.setFilterMusic(AudioPlayer::Filter::LowPass);
    mIsActive = true;
    mCursorOption = Button::Resume;
}

void PauseMenu::deactivate() {
    if (!mIsActive) {
        return;
    }
    System::setPaused(false);
    System::audio.setFilterMusic(mPrevMusicFilter);
    mIsActive = false;
}

}  // namespace whal

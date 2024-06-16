#include "PauseMenu.h"
#include <raylib.h>

#include "Events/Events.h"
#include "Settings.h"
#include "System.h"

namespace whal {

static const char* S_BUTTON_TO_NAME[] = {"Resume", "Exit"};

void buttonCallback(InputType input) {
    PauseMenu::instance().onButtonPressed(input);
}

s32 pythonMod(s32 a, s32 b) {
    // behaves like a % b in python (different from c++ for negative #s)
    return (b + (a % b)) % b;
}

PauseMenu::PauseMenu() : mInputListener(&buttonCallback) {
    System::eventMgr.registerListener(Event::BUTTON_EVENT_PRESS, mInputListener);
}

void PauseMenu::onButtonPressed(InputType input) {
    if (!isPaused()) {
        if (input == InputType::PAUSE) {
            pause();
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
        unpause();
        System::audio.playMenuClip(Sfx::MENU_CLOSE, 0.33);
        break;
    case InputType::OK:
        doCursorAction();
        System::audio.playMenuClip(Sfx::MENU_SELECT, 0.33);
        break;
    default:
        break;
    }
}

void PauseMenu::draw(Font* font) const {
    if (!isPaused()) {
        return;
    }
    constexpr s32 spacing = 24;   // PARAM
    constexpr s32 fontSize = 48;  // PARAM
    constexpr s32 spacingX = 0;   // PARAM

    constexpr s32 lineheight = spacing + fontSize;
    constexpr s32 menuHeight = lineheight * N_BUTTONS - spacing;  // n-1 fence posts
    constexpr s32 startHeight = WINDOW_HEIGHT_ACTUAL / 2 - menuHeight / 2;

    for (s32 i = 0; i < N_BUTTONS; i++) {
        const char* buttonText = S_BUTTON_TO_NAME[i];
        Color color = static_cast<Button>(i) == mCursorOption ? RED : WHITE;
        Vector2 textDimensions = MeasureTextEx(*font, buttonText, fontSize, spacingX);
        DrawTextEx(*font, buttonText, Vector2(WINDOW_WIDTH_ACTUAL / 2 - textDimensions.x / 2, startHeight + lineheight * i), fontSize, spacingX,
                   color);
    }
}

void PauseMenu::doCursorAction() {
    switch (mCursorOption) {
    case Button::Resume:
        unpause();
        break;
    case Button::Exit:
        System::quit();
        break;
    }
}

void PauseMenu::pause() {
    if (mIsPaused) {
        return;
    }
    System::setPaused(true);
    mPrevMusicFilter = System::audio.getFilterMusic();
    System::audio.setFilterMusic(AudioPlayer::Filter::LowPass);
    mIsPaused = true;
    mCursorOption = Button::Resume;
}

void PauseMenu::unpause() {
    if (!mIsPaused) {
        return;
    }
    System::setPaused(false);
    System::audio.setFilterMusic(mPrevMusicFilter);
    mIsPaused = false;
}

}  // namespace whal

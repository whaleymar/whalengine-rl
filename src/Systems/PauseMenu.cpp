#include "PauseMenu.h"
#include <raylib.h>

#include "Game/Events.h"
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
    System::eventMgr.registerListener(Event::BUTTON_EVENT, mInputListener);
}

void PauseMenu::onButtonPressed(InputType input) {
    if (!isPaused()) {
        if (input == InputType::PAUSE) {
            pause();
            System::audio.playMenuClip(Sfx::MENU_OPEN);
        }
        return;
    }

    switch (input) {
    case InputType::UP:
        mCursorOption = static_cast<Button>(pythonMod(mCursorOption - 1, N_BUTTONS));
        System::audio.playMenuClip(Sfx::MENU_MOVE);
        break;
    case InputType::DOWN:
        mCursorOption = static_cast<Button>(pythonMod(mCursorOption + 1, N_BUTTONS));
        System::audio.playMenuClip(Sfx::MENU_MOVE);
        break;
    case InputType::PAUSE:
        unpause();
        System::audio.playMenuClip(Sfx::MENU_CLOSE);
        break;
    case InputType::OK:
        doCursorAction();
        System::audio.playMenuClip(Sfx::MENU_SELECT);
        break;
    default:
        break;
    }
}

void PauseMenu::draw() const {
    if (!isPaused()) {
        return;
    }
    constexpr s32 xOffset = -40;  // PARAM
    constexpr s32 spacing = 24;   // PARAM
    constexpr s32 fontSize = 48;  // PARAM

    constexpr s32 lineheight = spacing + fontSize;
    constexpr s32 menuHeight = lineheight * N_BUTTONS - spacing;  // n-1 fence posts
    constexpr s32 startHeight = WINDOW_HEIGHT_ACTUAL / 2 - menuHeight / 2;

    for (s32 i = 0; i < N_BUTTONS; i++) {
        const char* buttonText = S_BUTTON_TO_NAME[i];
        Color color = static_cast<Button>(i) == mCursorOption ? RED : WHITE;
        DrawText(buttonText, WINDOW_WIDTH_ACTUAL / 2 + xOffset, startHeight + lineheight * i, fontSize, color);
    }
}

void PauseMenu::doCursorAction() {
    switch (mCursorOption) {
    case Button::Resume:
        unpause();
        break;
    case Button::Exit:
        // TODO
        break;
    }
}

void PauseMenu::pause() {
    System::setPaused(true);
    System::audio.setFilterMusic(AudioPlayer::Filter::LowPass);
    mIsPaused = true;
    mCursorOption = Button::Resume;
}

void PauseMenu::unpause() {
    System::setPaused(false);
    System::audio.setFilterMusic(AudioPlayer::Filter::None);
    mIsPaused = false;
}

}  // namespace whal

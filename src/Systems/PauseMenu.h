#pragma once

#include "Event.h"
#include "Systems/Audio.h"
#include "Util/Types.h"

typedef struct Font Font;

namespace whal {

enum class InputType : u64;

class PauseMenu {
    enum Button { Resume, Exit };
    inline static constexpr s32 N_BUTTONS = 2;

public:
    static PauseMenu& instance() {
        static PauseMenu instance_;
        return instance_;
    }

    void onButtonPressed(InputType input);
    void draw(Font* font) const;
    bool isActive() const { return mIsActive; }

private:
    PauseMenu();
    PauseMenu(const PauseMenu&) = delete;
    void operator=(const PauseMenu&) = delete;

    void doCursorAction();
    void deactivate();
    void activate();

    EventListener<InputType> mInputListener;
    Button mCursorOption = Button::Resume;
    AudioPlayer::Filter mPrevMusicFilter;
    bool mIsActive = false;
};

}  // namespace whal

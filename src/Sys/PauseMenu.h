#pragma once

#include "Audio.h"
#include "Events/Events.h"
#include "System.h"
#include "Util/Types.h"

typedef struct Font Font;

namespace whal {

enum class InputType : u64;

class PauseMenu : public IListen<ButtonPressEvent, true, InputType> {
    enum Button { Resume, Restart, FullScreenToggle, Exit };
    inline static constexpr s32 N_BUTTONS = 4;

public:
    static PauseMenu& instance() {
        static PauseMenu instance_;
        return instance_;
    }

    void onEvent(whal::ButtonPressEvent, InputType input) override;
    void draw(Font& font) const;
    bool isActive() const { return mIsActive; }

private:
    PauseMenu() = default;
    PauseMenu(const PauseMenu&) = delete;
    void operator=(const PauseMenu&) = delete;

    void doCursorAction();
    void deactivate();
    void activate();

    Button mCursorOption = Button::Resume;
    AudioPlayer::Filter mPrevMusicFilter = AudioPlayer::Filter::None;
    bool mIsActive = false;
};

}  // namespace whal

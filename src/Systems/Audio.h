#pragma once

#include <optional>

#include "Util/Types.h"
#include "whalECS/src/Expected.h"

namespace FMOD {

class System;
class Sound;
class Channel;
class ChannelGroup;

}  // namespace FMOD

namespace whal {

static constexpr s32 MAX_CHANNELS = 256;  // upper limit. just used for AudioPlayer's channel pool

struct System;

class AudioClip {
public:
    AudioClip() = default;
    AudioClip(const char* path);
    ~AudioClip();

    AudioClip(const AudioClip&) = delete;
    void operator=(const AudioClip&) = delete;

    std::optional<Error> load(const char* path);
    bool isValid() const { return mSound != nullptr; }
    FMOD::Sound* get() const { return mSound; }

private:
    FMOD::Sound* mSound = nullptr;
};

class AudioPlayer {
public:
    friend System;
    friend AudioClip;

    void playMusic(const char* path, f32 volume = 1.0);
    void playClip(const AudioClip& clip, f32 volume = 1.0);
    void stopMusic();
    void stopClips();
    void stopAll();
    bool isMusicPaused() const;
    bool isClipsPaused() const;
    void pauseMusic(bool pause);
    void pauseClips(bool pause);
    void pauseAll(bool pause);
    void setMusicVolume(f32 volume);
    bool isValid() const { return mIsValid; }
    void update();

private:
    AudioPlayer();
    ~AudioPlayer();

    AudioPlayer(const AudioPlayer&) = delete;
    void operator=(const AudioPlayer&) = delete;

    FMOD::System* getSystem() const;

    FMOD::Sound* mMusic = nullptr;
    FMOD::ChannelGroup* mClipChannelGroup = nullptr;
    FMOD::Channel* mClipChannelPool[MAX_CHANNELS];
    FMOD::Channel* mMusicChannel = nullptr;
    FMOD::System* mSystem = nullptr;
    s32 mMaxChannelCount = 0;
    s32 mNumMusicChannels = 1;
    s32 mNumClipChannels = 0;
    bool mIsValid = false;
    bool mIsPlayingMusic = false;
    bool mIsPlayingChannels = false;
};

class Sfx {
public:
    static Sfx& instance() {
        static Sfx instance_;
        return instance_;
    }

    inline static AudioClip GAMEOVER;
    inline static AudioClip EXPLOSION;
    inline static AudioClip FOOTSTEPTEST;
    inline static AudioClip SHOTFIRED;
    inline static AudioClip JUMP;
    inline static AudioClip LAND;
    inline static AudioClip DEATH;

    std::optional<Error> load();

private:
    Sfx() = default;
    Sfx(Sfx& other) = delete;

    inline static bool mIsLoaded = false;
};

}  // namespace whal

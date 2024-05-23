#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#include <condition_variable>
#include <mutex>
#include <optional>

#include "Util/Types.h"
#include "whalECS/src/Expected.h"

namespace whal {

struct System;

class Music {
public:
    Music(const char* path);
    ~Music();

    Music(const Music&) = delete;
    void operator=(const Music&) = delete;

    bool isValid() const { return mData != nullptr; }
    Mix_Music* get() const { return mData; }

private:
    const char* mPath = nullptr;
    Mix_Music* mData = nullptr;
};

class AudioClip {
public:
    AudioClip() = default;
    AudioClip(const char* path);
    ~AudioClip();

    AudioClip(const AudioClip&) = delete;
    void operator=(const AudioClip&) = delete;

    std::optional<Error> load(const char* path);
    bool isValid() const { return mData != nullptr; }
    Mix_Chunk* get() const { return mData; }

private:
    const char* mPath;
    Mix_Chunk* mData = nullptr;
};

// TODO should be able to set volume
class AudioPlayer {
public:
    friend System;

    void start();
    void await();
    void end();

    // void play(const Music& music);
    void playMusic(const char* path);
    void play(const AudioClip& clip, f32 volume = 1.0) const;
    void stopMusic();
    void stopAll();
    bool isValid() const { return mIsValid; }

private:
    AudioPlayer();
    ~AudioPlayer();

    AudioPlayer(const AudioPlayer&) = delete;
    void operator=(const AudioPlayer&) = delete;

    void playerThread();

    std::mutex mMutex;
    std::thread mMusicThread;
    std::condition_variable mCondition;
    const Music* mQueuedMusic = nullptr;
    std::optional<Music> mMusic;
    bool mIsValid = false;
    bool mIsMusicStopSignal = false;
    bool mIsTerminated = false;
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

    std::optional<Error> load();

private:
    Sfx() = default;
    Sfx(Sfx& other) = delete;

    inline static bool mIsLoaded = false;
};

}  // namespace whal

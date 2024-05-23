#pragma once

#include <condition_variable>
#include <mutex>
#include <optional>

#include "Util/Types.h"
#include "whalECS/src/Expected.h"

typedef struct Music Music;

namespace whal {

struct System;

class MusicClip {
public:
    MusicClip(const char* path);
    ~MusicClip();

    MusicClip(const MusicClip&) = delete;
    void operator=(const MusicClip&) = delete;

    bool isValid() const;
    Music* get() const { return mMusic; };

private:
    Music* mMusic;
    bool mIsValid;
};

class AudioClip {
public:
    AudioClip() = default;
    AudioClip(const char* path);
    // ~AudioClip();

    AudioClip(const AudioClip&) = delete;
    void operator=(const AudioClip&) = delete;

    bool isValid() const;

private:
    const char* mPath;
    bool mIsValid = false;
};

class AudioPlayer {
public:
    friend System;

    void start();
    void await();
    void end();
    void update();

    void playMusic(const char* path);
    void play(const AudioClip& clip, f32 volume = 1.0) const;
    void stopMusic();
    void stopAll();
    // bool isValid() const { return mIsValid; }

private:
    AudioPlayer();

    // AudioPlayer(const AudioPlayer&) = delete;
    // void operator=(const AudioPlayer&) = delete;

    void playerThread();

    std::mutex mMutex;
    std::thread mMusicThread;
    std::condition_variable mCondition;
    std::optional<MusicClip> mQueuedMusic;
    // bool mIsValid = false;
    bool mIsMusicStopSignal = false;
    bool mIsTerminated = false;
    bool mIsUpdateSignal = false;
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

#include "Audio.h"

#include <SDL2/SDL_mixer.h>
#include <thread>

#include "Util/Print.h"

namespace whal {

Music::Music(const char* path) {
    mPath = path;
    mData = Mix_LoadMUS(path);
    if (mData == nullptr) {
        print(Mix_GetError());
        mData = nullptr;
        mPath = nullptr;
    }
}

Music::~Music() {
    if (mData) {
        Mix_FreeMusic(mData);
    }
}

AudioClip::AudioClip(const char* path) {
    if (auto errOpt = load(path); errOpt) {
        print("Failed to load audio clip:", path, "\nGot error:", errOpt.value());
    }
}

AudioClip::~AudioClip() {
    if (mData) {
        Mix_FreeChunk(mData);
    }
}

std::optional<Error> AudioClip::load(const char* path) {
    if (isValid()) {
        return Error("Cannot load path into already loaded AudioClip");
    }
    mPath = path;
    // can be mp3 or wav (or others?)
    mData = Mix_LoadWAV(path);
    if (mData == nullptr) {
        return Error(Mix_GetError());
    }
    return std::nullopt;
}

AudioPlayer::AudioPlayer() {
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 8, 2048) < 0) {
        return;
    }
    mIsValid = true;

    if (auto errOpt = Sfx::instance().load(); errOpt) {
        print(errOpt.value());
    }
}

AudioPlayer::~AudioPlayer() {
    if (mIsValid) {
        Mix_CloseAudio();
    }
}

void AudioPlayer::start() {
    mMusicThread = std::thread(&AudioPlayer::playerThread, this);
}

void AudioPlayer::await() {
    mMusicThread.join();
}

void AudioPlayer::end() {
    mIsTerminated = true;
    mCondition.notify_one();
}

void AudioPlayer::playMusic(const char* path) {
    mMusic.emplace(path);

    if (!mMusic || !mMusic->isValid()) {
        return;
    }

    mQueuedMusic = &mMusic.value();
    mCondition.notify_one();
}

// plays an audio clip. Can pass in desired volume scale between 0-1. Default 1
void AudioPlayer::play(const AudioClip& clip, f32 volume) const {
    if (!clip.isValid()) {
        return;
    }

    // TODO should probably track which channels are being used + set some aside for ECS?
    // arg 1: -1 to choose any channel
    // arg 3:
    // 0 == play once and stop
    // -1 == play forever?

    // volume ranges from 0-128
    s32 sVolume = static_cast<s32>(128.0 * volume);
    auto channel = Mix_PlayChannel(-1, clip.get(), 0);
    if (channel == -1) {
        print("couldn't play audio clip");
        return;
    }
    Mix_Volume(channel, sVolume);
}

void AudioPlayer::stopMusic() {
    mIsMusicStopSignal = true;
    mCondition.notify_one();
}

void AudioPlayer::stopAll() {
    stopMusic();
    // TODO stop clips
}

void AudioPlayer::playerThread() {
    while (!mIsTerminated) {
        std::unique_lock<std::mutex> lock(mMutex);
        mCondition.wait(lock, [this] { return mQueuedMusic != nullptr || mIsTerminated || mIsMusicStopSignal; });
        if (mIsMusicStopSignal && Mix_PlayingMusic()) {
            Mix_HaltMusic();
            mQueuedMusic = nullptr;
            mIsMusicStopSignal = false;
        }
        if (mQueuedMusic == nullptr || !mQueuedMusic->isValid()) {
            mQueuedMusic = nullptr;
            continue;
        }

        // 0 == play once and stop
        // -1 == play forever?
        Mix_PlayMusic(mQueuedMusic->get(), 0);
        mQueuedMusic = nullptr;
    }
}

std::optional<Error> Sfx::load() {
    if (mIsLoaded) {
        print("Already loaded sound effects. Skipping reload");
        return std::nullopt;
    }
    mIsLoaded = true;
    std::optional<Error> errOpt;

    errOpt = GAMEOVER.load("data/zeldaGameOverSound.mp3");
    if (errOpt)
        return errOpt;

    errOpt = EXPLOSION.load("data/audio/sfx/explosion.wav");
    if (errOpt)
        return errOpt;

    errOpt = FOOTSTEPTEST.load("data/audio/sfx/test-footstep.mp3");
    if (errOpt) {
        return errOpt;
    }

    errOpt = SHOTFIRED.load("data/audio/sfx/shotfired.wav");
    if (errOpt) {
        return errOpt;
    }

    return std::nullopt;
}

}  // namespace whal

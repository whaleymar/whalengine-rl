#include "Audio.h"

#include <raylib.h>
#include "Util/Print.h"

namespace whal {

MusicClip::MusicClip(const char* path) {
    mMusic = new Music(LoadMusicStream(path));
    if (IsMusicReady(*mMusic)) {
        mIsValid = true;
    } else {
        mIsValid = false;
    }
}

MusicClip::~MusicClip() {
    if (isValid()) {
        if (IsMusicStreamPlaying(*mMusic)) {
            StopMusicStream(*mMusic);
        }
        UnloadMusicStream(*mMusic);
        delete mMusic;
    }
}

bool MusicClip::isValid() const {
    return mIsValid;
}

AudioClip::AudioClip(const char* path) {}

bool AudioClip::isValid() const {
    return mIsValid;
}

AudioPlayer::AudioPlayer() {
    // if (auto errOpt = Sfx::instance().load(); errOpt) {
    //     print(errOpt.value());
    // }
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

void AudioPlayer::update() {
    mIsUpdateSignal = true;
    mCondition.notify_one();
}

void AudioPlayer::playMusic(const char* path) {
    mQueuedMusic.emplace(path);  // automatically calls destructor on old music if it exists
    mCondition.notify_one();
}

// plays an audio clip. Can pass in desired volume scale between 0-1. Default 1
void AudioPlayer::play(const AudioClip& clip, f32 volume) const {
    if (!clip.isValid()) {
        return;
    }
    // TODO
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
        mCondition.wait(lock, [this] { return mQueuedMusic || mIsTerminated || mIsMusicStopSignal || mIsUpdateSignal; });

        // check termination
        if (mIsTerminated) {
            continue;
        }

        // check if music stopped
        if (mIsMusicStopSignal && IsMusicStreamPlaying(*mQueuedMusic->get())) {
            StopMusicStream(*mQueuedMusic->get());
            mQueuedMusic.reset();
            mQueuedMusic = std::nullopt;  // this probably happens automatically but I don't see it in the documentation
            mIsMusicStopSignal = false;
        }

        if (mIsUpdateSignal && mQueuedMusic) {
            UpdateMusicStream(*mQueuedMusic->get());
        }

        // check if we have valid music queued
        if (mQueuedMusic && !mQueuedMusic->isValid()) {
            mQueuedMusic.reset();
            mQueuedMusic = std::nullopt;  // this probably happens automatically but I don't see it in the documentation
            continue;
        } else if (!mQueuedMusic) {
            continue;
        }

        PlayMusicStream(*mQueuedMusic->get());
    }
}

}  // namespace whal

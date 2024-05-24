#include "Audio.h"

#include <fmod.hpp>
#include "fmod_common.h"

#include "System.h"
#include "Util/Print.h"

namespace whal {

static const char* CHANNEL_GROUP_NAME_CLIPS = "Clips";

AudioClip::AudioClip(const char* path) {
    load(path);
}

AudioClip::~AudioClip() {
    if (isValid()) {
        mSound->release();
    }
}

std::optional<Error> AudioClip::load(const char* path) {
    auto result = System::audio.getSystem()->createSound(path, FMOD_DEFAULT, nullptr, &mSound);
    if (result != FMOD_OK) {
        mSound = nullptr;
        return Error("Error loading clip");
    }
    return std::nullopt;
}

AudioPlayer::AudioPlayer() {
    // Init System
    auto result = FMOD::System_Create(&mSystem);
    if (result != FMOD_OK) {
        print("Got bad result for System_Create");
        return;
    }
    auto outputSettings = FMOD_OUTPUTTYPE_AUTODETECT;
    result = mSystem->init(MAX_CHANNELS, FMOD_INIT_NORMAL, &outputSettings);
    if (result != FMOD_OK) {
        print("Got bad result for mSystem->init");
        return;
    }
    mIsValid = true;

    // Init Clip Channel Pool
    mSystem->getSoftwareChannels(&mMaxChannelCount);
    print("AudioPlayer has", mMaxChannelCount, "channels");
    if (mMaxChannelCount > MAX_CHANNELS) {
        mMaxChannelCount = MAX_CHANNELS;
    }
    mNumClipChannels = mMaxChannelCount - mNumMusicChannels;
    mSystem->createChannelGroup(CHANNEL_GROUP_NAME_CLIPS, &mClipChannelGroup);
    for (s32 i = 0; i < mNumClipChannels; i++) {
        mClipChannelPool[i] = nullptr;
    }

    // Init Sfx Clips
    if (auto errOpt = Sfx::instance().load(); errOpt) {
        print(errOpt.value());
    }
}

AudioPlayer::~AudioPlayer() {
    if (mIsValid) {
        mSystem->close();
        mSystem->release();
    }
}

void AudioPlayer::playMusic(const char* path, f32 volume) {
    if (!mIsValid) {
        return;
    }

    auto result = mSystem->createStream(path, FMOD_DEFAULT, nullptr, &mMusic);
    if (result != FMOD_OK) {
        print("couldn't load music stream:", path);
    }
    mSystem->playSound(mMusic, nullptr, false, &mMusicChannel);
    mMusicChannel->setVolume(volume);
    mIsPlayingMusic = true;
}

void AudioPlayer::update() {
    // update music
    if (mIsPlayingMusic || mIsPlayingChannels) {
        mSystem->update();
    }

    if (mIsPlayingMusic) {
        mMusicChannel->isPlaying(&mIsPlayingMusic);
    } else if (mMusic != nullptr) {
        stopMusic();
    }

    if (mIsPlayingChannels) {
        bool isPlayingAnyClip = false;
        bool isPlaying = false;
        for (s32 i = 0; i < mNumClipChannels; i++) {
            FMOD::Channel* channel = mClipChannelPool[i];
            if (channel == nullptr) {
                continue;
            }
            channel->isPlaying(&isPlaying);
            isPlayingAnyClip = isPlayingAnyClip || isPlaying;

            if (!isPlaying) {
                channel->stop();
                mClipChannelPool[i] = nullptr;
            }
        }

        mIsPlayingChannels = isPlayingAnyClip;
    }
}

FMOD::System* AudioPlayer::getSystem() const {
    return mSystem;
}

// plays an audio clip. Can pass in desired volume scale between 0-1. Default 1
void AudioPlayer::playClip(const AudioClip& clip, f32 volume) {
    if (!clip.isValid()) {
        return;
    }

    s32 channelIx = -1;
    for (s32 i = 0; i < mNumClipChannels; i++) {
        if (mClipChannelPool[i] == nullptr) {
            channelIx = i;
            break;
        }
    }
    if (channelIx < 0) {
        print("Couldn't find channel available for clip");
        return;
    }

    FMOD::Channel** pChannel = &mClipChannelPool[channelIx];
    mSystem->playSound(clip.get(), mClipChannelGroup, false, pChannel);
    // don't loop
    (*pChannel)->setLoopCount(0);
    (*pChannel)->setVolume(volume);
    mIsPlayingChannels = true;
}

void AudioPlayer::stopMusic() {
    print("stopping music");
    if (mIsPlayingMusic) {
        mMusicChannel->stop();
    }
    mMusic->release();
    mMusic = nullptr;
    mIsPlayingMusic = false;
}

void AudioPlayer::stopClips() {
    if (!mIsPlayingChannels) {
        return;
    }

    mClipChannelGroup->stop();

    for (s32 i = 0; i < mNumClipChannels; i++) {
        mClipChannelPool[i] = nullptr;
    }
}

void AudioPlayer::stopAll() {
    stopMusic();
    stopClips();
}

void AudioPlayer::setMusicVolume(f32 volume) {
    if (mMusicChannel != nullptr && mIsPlayingMusic) {
        mMusicChannel->setVolume(volume);
    }
}

bool AudioPlayer::isMusicPaused() const {
    bool isPaused = false;
    mMusicChannel->getPaused(&isPaused);
    return isPaused;
}

bool AudioPlayer::isClipsPaused() const {
    bool isPaused = false;
    mClipChannelGroup->getPaused(&isPaused);
    return isPaused;
}

void AudioPlayer::pauseMusic(bool pause) {
    mMusicChannel->setPaused(pause);
}

void AudioPlayer::pauseClips(bool pause) {
    mClipChannelGroup->setPaused(pause);
}

void AudioPlayer::pauseAll(bool pause) {
    pauseMusic(pause);
    pauseClips(pause);
}

std::optional<Error> Sfx::load() {
    if (mIsLoaded) {
        print("Already loaded sound effects. Skipping reload");
        return std::nullopt;
    }
    mIsLoaded = true;
    std::optional<Error> errOpt;

    errOpt = GAMEOVER.load("data/audio/sfx/zeldaGameOverSound.mp3");
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

    errOpt = JUMP.load("data/audio/sfx/jump.wav");
    if (errOpt) {
        return errOpt;
    }

    errOpt = LAND.load("data/audio/sfx/land.wav");
    if (errOpt) {
        return errOpt;
    }

    errOpt = DEATH.load("data/audio/sfx/death.wav");
    if (errOpt) {
        return errOpt;
    }

    return std::nullopt;
}

}  // namespace whal

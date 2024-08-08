#include "Audio.h"

#ifndef __EMSCRIPTEN__

#include <fmod.hpp>
#include <fmod_errors.h>
#include "fmod_common.h"
#include "fmod_dsp_effects.h"

#include "Settings.h"
#include "System.h"

#else

#include <raylib.h>  // can remove, is in header

static const float EXPONENT = 1.0f;        // Audio exponentiation value
static float averageVolume[400] = {0.0f};  // Average volume history
void ProcessAudio(void* buffer, unsigned int frames) {
    float* samples = (float*)buffer;  // Samples internally stored as <float>s
    float average = 0.0f;             // Temporary average volume

    for (unsigned int frame = 0; frame < frames; frame++) {
        float *left = &samples[frame * 2 + 0], *right = &samples[frame * 2 + 1];

        *left = powf(fabsf(*left), EXPONENT) * ((*left < 0.0f) ? -1.0f : 1.0f);
        *right = powf(fabsf(*right), EXPONENT) * ((*right < 0.0f) ? -1.0f : 1.0f);

        average += fabsf(*left) / frames;  // accumulating average volume
        average += fabsf(*right) / frames;
    }

    // Moving history to the left
    for (int i = 0; i < 399; i++)
        averageVolume[i] = averageVolume[i + 1];

    averageVolume[399] = average;  // Adding last average value
}

#endif

#include "Util/Print.h"

#define NULLOPT Corrade::Containers::NullOpt;

namespace whal {

#ifndef __EMSCRIPTEN__
static const char* CHANNEL_GROUP_NAME_CLIPS = "Clips";
constexpr f32 ATTEN_DIST_MIN = 30;
constexpr f32 ATTEN_DIST_MAX = 200;
constexpr f32 DIST_UNITS = FPIXELS_PER_TILE;
#endif

AudioClip::AudioClip(const char* path) {
    load(path);
}

AudioClip::~AudioClip() {
    if (isValid()) {
#ifndef __EMSCRIPTEN__
        mSound->release();
#else
        UnloadSound(mSound);
#endif
    }
}

Corrade::Containers::Optional<Error> AudioClip::load(const char* path) {
#ifndef __EMSCRIPTEN__
    auto result = System::audio.getSystem()->createSound(path, FMOD_LOOP_NORMAL | FMOD_3D, nullptr,
                                                         &mSound);  // looping on by default bc documentation recommends it
    if (result != FMOD_OK) {
        mSound = nullptr;
        auto err = FMOD_ErrorString(result);
        return Error(sprint("Error loading clip:", path, "\nGot error:", err));
    }
#else
    mSound = LoadSound(path);
    if (!IsSoundReady(mSound)) {
        mIsValid = false;
        return Error(sprint("Error loading audio clip: ", path));
    }
    mIsValid = true;

#endif
    return NULLOPT;
}

Corrade::Containers::Optional<Error> AudioPlayer::init() {
#ifndef __EMSCRIPTEN__
    // Init System
    auto result = FMOD::System_Create(&mSystem);
    if (result != FMOD_OK) {
        return Error(sprint("Got bad result for System_Create:", FMOD_ErrorString(result)));
    }
    auto outputSettings = FMOD_OUTPUTTYPE_AUTODETECT;
    result = mSystem->init(MAX_CHANNELS, FMOD_INIT_NORMAL, &outputSettings);
    if (result != FMOD_OK) {
        return Error(sprint("Got bad result for mSystem->init:", FMOD_ErrorString(result)));
    }
    mIsValid = true;

    // Init Clip Channel Pool
    mSystem->getSoftwareChannels(&mMaxChannelCount);
    print("AudioPlayer has", mMaxChannelCount, "channels");
    if (mMaxChannelCount > MAX_CHANNELS) {
        mMaxChannelCount = MAX_CHANNELS;
    }
    mNumClipChannels = mMaxChannelCount - mNumMiscChannels;
    mSystem->createChannelGroup(CHANNEL_GROUP_NAME_CLIPS, &mClipChannelGroup);
    for (s32 i = 0; i < mNumClipChannels; i++) {
        mClipChannelPool[i] = nullptr;
    }

    s32 nListeners;
    mSystem->get3DNumListeners(&nListeners);
    if (nListeners != 1) {
        mSystem->set3DNumListeners(1);
    }
    mSystem->set3DSettings(0.0f, DIST_UNITS, 1.0f);

#else

    InitAudioDevice();
    if (!IsAudioDeviceReady()) {
        return Error("Audio device not ready");
    }
    AttachAudioMixedProcessor(ProcessAudio);
    mIsValid = true;

#endif
    // Init Sfx Clips
    if (auto errOpt = Sfx::instance().load(); errOpt) {
        return errOpt;
    }

    return NULLOPT;
}

AudioPlayer::AudioPlayer() {}

AudioPlayer::~AudioPlayer() {
    if (mIsValid) {
#ifndef __EMSCRIPTEN__
        if (mLowpassFilter != nullptr) {
            mLowpassFilter->release();
        }
        mSystem->close();
        mSystem->release();
#else
        DetachAudioMixedProcessor(ProcessAudio);
        CloseAudioDevice();
#endif
    }
}

void AudioPlayer::playMusic(const char* path, f32 volume, Filter filter, bool isLooping, Vector2i* position) {
    if (!mIsValid) {
        return;
    }

    stopMusic();

#ifndef __EMSCRIPTEN__
    auto result = mSystem->createStream(path, FMOD_LOOP_NORMAL, nullptr, &mMusic);
    if (result != FMOD_OK) {
        print("couldn't load music stream:", path, "\nGot error:", FMOD_ErrorString(result));
        return;
    }

    if (isLooping) {
        mMusicChannel->setLoopCount(-1);
        mMusicChannel->setMode(FMOD_LOOP_NORMAL);
    } else {
        mMusic->setMode(FMOD_LOOP_OFF);
        mMusicChannel->setMode(FMOD_LOOP_OFF);
    }
    mSystem->playSound(mMusic, nullptr, false, &mMusicChannel);
    mMusicChannel->setVolume(volume);
    setFilterMusic(filter);

    if (position != nullptr) {
        // 1 = 100% 3D, 0 = 100% 2D
        mMusicChannel->set3DLevel(0.75);                                     // mix of 2D and 3D sound. Only 3D sounds kinda weird IMO
        mMusicChannel->set3DMinMaxDistance(ATTEN_DIST_MIN, ATTEN_DIST_MAX);  // I probably want to set this per sound, not channel
        FMOD_VECTOR vec = FMOD_VECTOR(position->x, position->y, 0);
        result = mMusicChannel->set3DAttributes(&vec, nullptr);
        if (result != FMOD_OK) {
            print("Got error setting channel position: ", FMOD_ErrorString(result));
        }
    } else {
        mMusicChannel->set3DLevel(0.0);
    }

#else

    mMusic = LoadMusicStream(path);
    if (!IsMusicReady(mMusic)) {
        print("couldn't load music stream: ", path);
        return;
    }

    // looping? prob have to use seek() in update() TODO

    SetMusicVolume(mMusic, volume);
    PlayMusicStream(mMusic);

#endif

    mIsPlayingMusic = true;
}

void AudioPlayer::update() {
#ifndef __EMSCRIPTEN__
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

#else
    if (mIsPlayingMusic) {
        UpdateMusicStream(mMusic);
        if (!IsMusicStreamPlaying(mMusic)) {
            stopMusic();
        }
    }
    if (!mIsClipsPaused) {
        std::vector<Sound> clipsNew;
        for (auto sound : mClipSounds) {
            if (IsSoundPlaying(sound)) {
                clipsNew.push_back(sound);
            }
        }
        mClipSounds = std::move(clipsNew);
    }
    std::vector<Sound> clipsNew;
    for (auto sound : mMenuSounds) {
        if (IsSoundPlaying(sound)) {
            clipsNew.push_back(sound);
        }
    }
    mMenuSounds = std::move(clipsNew);
#endif
}

// plays an audio clip. Can pass in desired volume scale between 0-1. Default 1
void AudioPlayer::playClip(const AudioClip& clip, f32 volume, Filter filter, bool isLooping, Vector2i* position) {
    if (!clip.isValid()) {
        return;
    }

#ifndef __EMSCRIPTEN__
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
    playClipWithChannel(clip, *pChannel, volume, filter, isLooping, position);
#else
    SetSoundVolume(clip.get(), volume);
    PlaySound(clip.get());
    mClipSounds.push_back(clip.get());
#endif
}
void AudioPlayer::playMenuClip(const AudioClip& clip, f32 volume, Filter filter, bool isLooping) {
    if (!clip.isValid()) {
        return;
    }
#ifndef __EMSCRIPTEN__
    playClipWithChannel(clip, mMenuChannel, volume, filter, isLooping, nullptr, false);
#else
    SetSoundVolume(clip.get(), volume);
    PlaySound(clip.get());
    mMenuSounds.push_back(clip.get());
#endif
}

#ifndef __EMSCRIPTEN__
FMOD::System* AudioPlayer::getSystem() const {
    return mSystem;
}

void AudioPlayer::playClipWithChannel(const AudioClip& clip, FMOD::Channel* channel, f32 volume, Filter filter, bool isLooping, Vector2i* position,
                                      bool isInGroup) {
    if (isLooping) {
        // -1 -> loop forever
        // 0 -> don't loop
        // 1 -> loop once
        channel->setLoopCount(-1);  // RESEARCH channels also have a setMode function which takes a looping param

    } else {
        clip.get()->setMode(FMOD_LOOP_OFF);  // on by default
        channel->setLoopCount(0);
    }

    FMOD::ChannelGroup* group = nullptr;
    if (isInGroup) {
        group = mClipChannelGroup;
    }
    mSystem->playSound(clip.get(), group, false, &channel);
    channel->setVolume(volume);
    setChannelFilter(filter, channel);

    if (position != nullptr) {
        // 1 = 100% 3D, 0 = 100% 2D
        channel->set3DLevel(0.75);                                     // mix of 2D and 3D sound. Only 3D sounds kinda weird IMO
        channel->set3DMinMaxDistance(ATTEN_DIST_MIN, ATTEN_DIST_MAX);  // I probably want to set this per sound, not channel
        FMOD_VECTOR vec = FMOD_VECTOR(position->x, position->y, 0);
        auto result = channel->set3DAttributes(&vec, nullptr);
        if (result != FMOD_OK) {
            print("Got error setting channel position: ", FMOD_ErrorString(result));
        }
    } else {
        channel->set3DLevel(0.0);
    }

    mIsPlayingChannels = true;
}
#endif

void AudioPlayer::stopMusic() {
    print("stopping music");
#ifndef __EMSCRIPTEN__
    if (mIsPlayingMusic) {
        mMusicChannel->stop();
    }
    if (mMusic) {
        mMusic->release();
        mMusic = nullptr;
    }
#else
    if (mIsPlayingMusic) {
        UnloadMusicStream(mMusic);
    }
#endif
    mIsPlayingMusic = false;
}

void AudioPlayer::stopClips() {
#ifndef __EMSCRIPTEN__
    if (!mIsPlayingChannels) {
        return;
    }

    mClipChannelGroup->stop();

    for (s32 i = 0; i < mNumClipChannels; i++) {
        mClipChannelPool[i] = nullptr;
    }
#else

    for (auto sound : mClipSounds) {
        StopSound(sound);
    }

    for (auto sound : mMenuSounds) {
        StopSound(sound);
    }

#endif
}

void AudioPlayer::stopAll() {
    stopMusic();
    stopClips();
}

void AudioPlayer::setMusicVolume(f32 volume) {
#ifndef __EMSCRIPTEN__
    if (mMusicChannel != nullptr && mIsPlayingMusic) {
        mMusicChannel->setVolume(volume);
    }
#else
    if (mIsPlayingMusic && IsMusicStreamPlaying(mMusic)) {
        SetMusicVolume(mMusic, volume);
    }

#endif
}

void AudioPlayer::setListenerPosition(Vector2i worldPosition) {
#ifndef __EMSCRIPTEN__
    FMOD_VECTOR position = FMOD_VECTOR(worldPosition.x, worldPosition.y, 0);
    auto result = mSystem->set3DListenerAttributes(0, &position, nullptr, nullptr, nullptr);
    if (result != FMOD_OK) {
        print("Error setting AudioListener position: ", FMOD_ErrorString(result));
    }
#endif
}

bool AudioPlayer::isMusicPaused() const {
#ifndef __EMSCRIPTEN__
    bool isPaused = false;
    mMusicChannel->getPaused(&isPaused);
    return isPaused;
#else
    return mIsPlayingMusic && !IsMusicStreamPlaying(mMusic);
#endif
}

bool AudioPlayer::isClipsPaused() const {
#ifndef __EMSCRIPTEN__
    bool isPaused = false;
    mClipChannelGroup->getPaused(&isPaused);
    return isPaused;
#else
    return mIsClipsPaused;
#endif
}

void AudioPlayer::pauseMusic(bool pause) {
#ifndef __EMSCRIPTEN__
    mMusicChannel->setPaused(pause);
#else
    if (pause) {
        PauseMusicStream(mMusic);
    } else {
        ResumeMusicStream(mMusic);
    }
#endif
}

void AudioPlayer::pauseClips(bool pause) {
#ifndef __EMSCRIPTEN__
    mClipChannelGroup->setPaused(pause);
#else
    for (auto sound : mClipSounds) {
        if (pause) {
            PauseSound(sound);
        } else {
            ResumeSound(sound);
        }
    }
    mIsClipsPaused = pause;
#endif
}

void AudioPlayer::pauseAll(bool pause) {
    pauseMusic(pause);
    pauseClips(pause);
}

void AudioPlayer::setFilterMusic(Filter filter) {
#ifndef __EMSCRIPTEN__
    setChannelFilter(filter, mMusicChannel);
    mMusicFilter = filter;
#endif
}

void AudioPlayer::setFilterClips(Filter filter) {
#ifndef __EMSCRIPTEN__
    setChannelFilter(filter, mClipChannelGroup);
    mClipsFilter = filter;
#endif
}

#ifndef __EMSCRIPTEN__
// needs to be free'd with dsp->release();
Expected<FMOD::DSP*> AudioPlayer::createLowPassFilter(f32 cutoff, f32 resonance) {
    FMOD::DSP* dsp;
    auto result = mSystem->createDSPByType(FMOD_DSP_TYPE_LOWPASS, &dsp);
    if (result != FMOD_OK) {
        auto err = FMOD_ErrorString(result);
        return Error(sprint("Got error creating DSP:", err));
    }

    result = dsp->setParameterFloat(FMOD_DSP_LOWPASS_CUTOFF, cutoff);
    if (result != FMOD_OK) {
        return Error(sprint("Got error assigning lowpass cutoff:", FMOD_ErrorString(result)));
    }

    result = dsp->setParameterFloat(FMOD_DSP_LOWPASS_RESONANCE, resonance);
    if (result != FMOD_OK) {
        return Error(sprint("Got error assigning lowpass resonance:", FMOD_ErrorString(result)));
    }

    return dsp;
}

void AudioPlayer::setChannelFilter(Filter filter, FMOD::ChannelControl* channel) {
    FMOD::DSP* dsp = nullptr;
    switch (filter) {
    case Filter::None:
        break;
    case Filter::LowPass:
        if (mLowpassFilter == nullptr) {
            auto eDSP = createLowPassFilter();
            if (eDSP.isExpected()) {
                dsp = eDSP.value();
                mLowpassFilter = dsp;
            } else {
                print("got error creating low pass filter: ", eDSP.error());
            }
        } else {
            dsp = mLowpassFilter;
        }
    }

    if (dsp != nullptr) {
        channel->addDSP(0, dsp);  // TODO hard coded index
    } else {
        channel->getDSP(0, &dsp);
        if (dsp != nullptr) {
            channel->removeDSP(dsp);
        }
    }
}
#endif

Corrade::Containers::Optional<Error> Sfx::load() {
    if (mIsLoaded) {
        print("Already loaded sound effects. Skipping reload");
        return NULLOPT;
    }
    mIsLoaded = true;
    Corrade::Containers::Optional<Error> errOpt;

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

    errOpt = MENU_MOVE.load("data/audio/sfx/menu_move.wav");
    if (errOpt) {
        return errOpt;
    }

    errOpt = MENU_OPEN.load("data/audio/sfx/menu_open.wav");
    if (errOpt) {
        return errOpt;
    }

    errOpt = MENU_CLOSE.load("data/audio/sfx/menu_close.wav");
    if (errOpt) {
        return errOpt;
    }

    errOpt = MENU_SELECT.load("data/audio/sfx/menu_choose.wav");
    if (errOpt) {
        return errOpt;
    }

    errOpt = SWITCH_FLIP.load("data/audio/sfx/switch_flip.wav");
    if (errOpt) {
        return errOpt;
    }

    errOpt = DOOR_OPEN.load("data/audio/sfx/door_open.wav");
    if (errOpt) {
        return errOpt;
    }

    errOpt = MAJOR_ITEM_GET.load("data/audio/sfx/major_item_get.mp3");
    if (errOpt) {
        return errOpt;
    }

    return NULLOPT;
}

// AudioClip::AudioClip(const char* path) {}
//
// AudioClip::~AudioClip() {}
//
// Corrade::Containers::Optional<Error> AudioClip::load(const char* path) {
//     return NULLOPT;
// }
//
// AudioPlayer::AudioPlayer() {
//     mIsValid = true;
// }
//
// AudioPlayer::~AudioPlayer() {}
//
// void AudioPlayer::playMusic(const char* path, f32 volume, Filter filter, bool isLooping, Vector2i* position) {}
//
// void AudioPlayer::update() {}
//
// FMOD::System* AudioPlayer::getSystem() const {
//     return mSystem;
// }
//
// // plays an audio clip. Can pass in desired volume scale between 0-1. Default 1
// void AudioPlayer::playClip(const AudioClip& clip, f32 volume, Filter filter, bool isLooping, Vector2i* position) {}
// void AudioPlayer::playMenuClip(const AudioClip& clip, f32 volume, Filter filter, bool isLooping) {}
//
// void AudioPlayer::playClipWithChannel(const AudioClip& clip, FMOD::Channel* channel, f32 volume, Filter filter, bool isLooping, Vector2i* position,
//                                       bool isInGroup) {}
//
// void AudioPlayer::stopMusic() {}
//
// void AudioPlayer::stopClips() {}
//
// void AudioPlayer::stopAll() {}
//
// void AudioPlayer::setMusicVolume(f32 volume) {}
//
// void AudioPlayer::setListenerPosition(Vector2i worldPosition) {}
//
// bool AudioPlayer::isMusicPaused() const {
//     bool isPaused = false;
//     return isPaused;
// }
//
// bool AudioPlayer::isClipsPaused() const {
//     bool isPaused = false;
//     return isPaused;
// }
//
// void AudioPlayer::pauseMusic(bool pause) {}
//
// void AudioPlayer::pauseClips(bool pause) {}
//
// void AudioPlayer::pauseAll(bool pause) {}
//
// // needs to be free'd with dsp->release();
// Expected<FMOD::DSP*> AudioPlayer::createLowPassFilter(f32 cutoff, f32 resonance) {
//     return Error("stubbed");
// }
//
// void AudioPlayer::setFilterMusic(Filter filter) {}
//
// void AudioPlayer::setFilterClips(Filter filter) {}
//
// void AudioPlayer::setChannelFilter(Filter filter, FMOD::ChannelControl* channel) {}
//
// Corrade::Containers::Optional<Error> Sfx::load() {
//     return NULLOPT;
// }

}  // namespace whal

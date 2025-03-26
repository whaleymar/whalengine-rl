#include "Audio.h"

#include <fmod.h>
#include <fmod_errors.h>
#include "fmod_common.h"
#include "fmod_dsp_effects.h"

#include "Settings.h"
#include "System.h"

#include "Util/Print.h"

#define NULLOPT Corrade::Containers::NullOpt;

namespace whal {

static const char* CHANNEL_GROUP_NAME_CLIPS = "Clips";
constexpr f32 ATTEN_DIST_MIN = 30;
constexpr f32 ATTEN_DIST_MAX = 200;
constexpr f32 DIST_UNITS = FPIXELS_PER_TILE;

Expected<AudioClip> AudioClip::from(const char* path) {
    AudioClip clip;
    auto errOpt = clip.load(path);
    if (errOpt) {
        return *errOpt;
    } else {
        return clip;
    }
}

void AudioClip::unload() {
    if (isValid()) {
        FMOD_Sound_Release(mSound);
        mSound = nullptr;
    }
}

Corrade::Containers::Optional<Error> AudioClip::load(const char* path) {
    unload();
    auto result = FMOD_System_CreateSound(Audio.getSystem(), path, FMOD_LOOP_NORMAL | FMOD_3D, nullptr,
                                          &mSound);  // looping on by default bc documentation recommends it
    if (result != FMOD_OK) {
        mSound = nullptr;
        auto err = FMOD_ErrorString(result);
        return Error(sprint("Error loading clip:", path, "\nFMOD error:", err));
    }
    return NULLOPT;
}

Corrade::Containers::Optional<Error> AudioPlayer::init() {
    // Init System
    FMOD_RESULT result = FMOD_System_Create(&mSystem, FMOD_VERSION);
    if (result != FMOD_OK) {
        return Error(sprint("Got bad result for System_Create:", FMOD_ErrorString(result)));
    }
    auto outputSettings = FMOD_OUTPUTTYPE_AUTODETECT;
    result = FMOD_System_Init(mSystem, MAX_CHANNELS, FMOD_INIT_NORMAL, &outputSettings);
    if (result != FMOD_OK) {
        return Error(sprint("Got bad result for mSystem->init:", FMOD_ErrorString(result)));
    }
    mIsValid = true;

    // Init Clip Channel Pool
    FMOD_System_GetSoftwareChannels(mSystem, &mMaxChannelCount);
    print("AudioPlayer has", mMaxChannelCount, "channels");
    if (mMaxChannelCount > MAX_CHANNELS) {
        mMaxChannelCount = MAX_CHANNELS;
    }
    mNumClipChannels = mMaxChannelCount - mNumMiscChannels;
    FMOD_System_CreateChannelGroup(mSystem, CHANNEL_GROUP_NAME_CLIPS, &mClipChannelGroup);
    for (s32 i = 0; i < mNumClipChannels; i++) {
        mClipChannelPool[i] = nullptr;
    }

    s32 nListeners;
    FMOD_System_Get3DNumListeners(mSystem, &nListeners);
    if (nListeners != 1) {
        FMOD_System_Set3DNumListeners(mSystem, 1);
    }
    FMOD_System_Set3DSettings(mSystem, 0.0f, DIST_UNITS, 1.0f);

    return NULLOPT;
}

AudioPlayer::AudioPlayer() {}

void AudioPlayer::end() {
    if (mIsValid) {
        if (mLowpassFilter != nullptr) {
            FMOD_DSP_Release(mLowpassFilter);
            mLowpassFilter = nullptr;
        }
        FMOD_System_Close(mSystem);
        FMOD_System_Release(mSystem);
        mSystem = nullptr;
    }
}

void AudioPlayer::disable() {
    mDisabledState = {
        .isMusicPaused = isMusicPaused(),
        .isSfxPaused = isClipsPaused(),
    };
    pauseAll(true);
}

void AudioPlayer::enable() {
    if (!mDisabledState.isMusicPaused) {
        pauseMusic(false);
    }
    if (!mDisabledState.isSfxPaused) {
        pauseClips(false);
    }
}

void AudioPlayer::playMusic(const char* path, f32 volume, Filter filter, bool isLooping, Vector2i* position) {
    if (!mIsValid) {
        return;
    }

    stopMusic();

    auto result = FMOD_System_CreateStream(mSystem, path, FMOD_LOOP_NORMAL, nullptr, &mMusic);
    if (result != FMOD_OK) {
        print("couldn't load music stream:", path, "\nGot error:", FMOD_ErrorString(result));
        return;
    }

    if (isLooping) {
        FMOD_Channel_SetLoopCount(mMusicChannel, -1);
        FMOD_Channel_SetMode(mMusicChannel, FMOD_LOOP_NORMAL);
    } else {
        FMOD_Sound_SetMode(mMusic, FMOD_LOOP_OFF);
        FMOD_Channel_SetMode(mMusicChannel, FMOD_LOOP_OFF);
    }

    // initialize paused, set effects, then unpause

    FMOD_System_PlaySound(mSystem, mMusic, nullptr, true, &mMusicChannel);
    FMOD_Channel_SetVolume(mMusicChannel, mMasterVolume * mMusicVolume * volume);
    FMOD_Channel_SetMute(mMusicChannel, mIsMusicMuted);
    setFilterMusic(filter);
    FMOD_Channel_SetPaused(mMusicChannel, false);

    if (position != nullptr) {
        // 1 = 100% 3D, 0 = 100% 2D
        FMOD_Channel_Set3DLevel(mMusicChannel, 0.75);                                     // mix of 2D and 3D sound. Only 3D sounds kinda weird IMO
        FMOD_Channel_Set3DMinMaxDistance(mMusicChannel, ATTEN_DIST_MIN, ATTEN_DIST_MAX);  // I probably want to set this per sound, not channel
        FMOD_VECTOR vec = FMOD_VECTOR(position->x, position->y, 0);
        result = FMOD_Channel_Set3DAttributes(mMusicChannel, &vec, nullptr);
        if (result != FMOD_OK) {
            print("Got error setting channel position: ", FMOD_ErrorString(result));
        }
    } else {
        FMOD_Channel_Set3DLevel(mMusicChannel, 0.0);
    }

    // playing around with changing the playback speed
    // f32 freq;
    // FMOD_Channel_GetFrequency(mMusicChannel, &freq);
    // print("default frequency (from channel) is ", freq);
    // FMOD_Sound_GetDefaults(mMusic, &freq, nullptr);
    // print("default frequency (from sound) is ", freq);  // this will stay the same no matter what I do to the channel
    // FMOD_Channel_SetFrequency(mMusicChannel, freq * 0.5f);

    mIsPlayingMusic = true;
}

void AudioPlayer::update() {
    // update music
    if (mIsPlayingMusic || mIsPlayingChannels) {
        FMOD_System_Update(mSystem);
    }

    if (mIsPlayingMusic) {
        FMOD_BOOL isPlaying;
        FMOD_Channel_IsPlaying(mMusicChannel, &isPlaying);
        mIsPlayingMusic = isPlaying;
    } else if (mMusic != nullptr) {
        stopMusic();
    }

    if (mIsPlayingChannels) {
        bool isPlayingAnyClip = false;
        FMOD_BOOL isPlaying = false;
        for (s32 i = 0; i < mNumClipChannels; i++) {
            FMOD_CHANNEL* channel = mClipChannelPool[i];
            if (channel == nullptr) {
                continue;
            }
            FMOD_Channel_IsPlaying(channel, &isPlaying);
            isPlayingAnyClip = isPlayingAnyClip || isPlaying;

            if (!isPlaying) {
                FMOD_Channel_Stop(channel);
                mClipChannelPool[i] = nullptr;
            }
        }

        mIsPlayingChannels = isPlayingAnyClip;
    }
}

// plays an audio clip. Can pass in desired volume scale between 0-1. Default 1
void AudioPlayer::playClip(const AudioClip& clip, f32 volume, Filter filter, bool isLooping, Vector2i* position) {
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

    FMOD_CHANNEL** pChannel = &mClipChannelPool[channelIx];
    playClipWithChannel(clip, *pChannel, volume, filter, isLooping, position, 0.1);
}

void AudioPlayer::playMenuClip(const AudioClip& clip, f32 volume, Filter filter, bool isLooping) {
    if (!clip.isValid()) {
        return;
    }
    playClipWithChannel(clip, mMenuChannel, volume, filter, isLooping, nullptr, 0.0, false);
}

void AudioPlayer::playClip(const std::string& clipname, f32 volume, Filter filter, bool isLooping, Vector2i* position) {
    playClip(getClip(clipname.c_str()), volume, filter, isLooping, position);
}

void AudioPlayer::playMenuClip(const std::string& clipname, f32 volume, Filter filter, bool isLooping) {
    playMenuClip(getClip(clipname.c_str()), volume, filter, isLooping);
}

FMOD_SYSTEM* AudioPlayer::getSystem() const {
    return mSystem;
}

void AudioPlayer::playClipWithChannel(const AudioClip& clip, FMOD_CHANNEL* channel, f32 volume, Filter filter, bool isLooping, Vector2i* position,
                                      f32 maxPitchShift, bool isInGroup) {
    if (isLooping) {
        // -1 -> loop forever
        // 0 -> don't loop
        // 1 -> loop once
        FMOD_Channel_SetLoopCount(channel, -1);  // RESEARCH channels also have a setMode function which takes a looping param

    } else {
        FMOD_Sound_SetMode(clip.get(), FMOD_LOOP_OFF);  // on by default
        FMOD_Channel_SetLoopCount(channel, 0);
    }

    FMOD_CHANNELGROUP* group = nullptr;
    if (isInGroup) {
        group = mClipChannelGroup;
    }

    // initialize paused, apply affects, then unpause
    FMOD_System_PlaySound(mSystem, clip.get(), group, true, &channel);
    FMOD_Channel_SetVolume(channel, mMasterVolume * mSfxVolume * volume);
    FMOD_Channel_SetMute(channel, isSfxMuted());
    setChannelFilter(filter, channel);
    FMOD_Channel_SetPitch(channel, Rng.range(1.0f - maxPitchShift, 1.0f + maxPitchShift));
    FMOD_Channel_SetPaused(channel, false);

    if (position != nullptr) {
        // 1 = 100% 3D, 0 = 100% 2D
        FMOD_Channel_Set3DLevel(channel, 0.75);                                     // mix of 2D and 3D sound. Only 3D sounds kinda weird IMO
        FMOD_Channel_Set3DMinMaxDistance(channel, ATTEN_DIST_MIN, ATTEN_DIST_MAX);  // I probably want to set this per sound, not channel
        FMOD_VECTOR vec = FMOD_VECTOR(position->x, position->y, 0);
        auto result = FMOD_Channel_Set3DAttributes(channel, &vec, nullptr);
        if (result != FMOD_OK) {
            print("Got error setting channel position: ", FMOD_ErrorString(result));
        }
    } else {
        FMOD_Channel_Set3DLevel(channel, 0.0);
    }

    mIsPlayingChannels = true;
}

void AudioPlayer::stopMusic() {
    if (mIsPlayingMusic) {
        FMOD_Channel_Stop(mMusicChannel);
    }
    if (mMusic) {
        FMOD_Sound_Release(mMusic);
        mMusic = nullptr;
    }
    mIsPlayingMusic = false;
}

void AudioPlayer::stopClips() {
    if (!mIsPlayingChannels) {
        return;
    }

    FMOD_ChannelGroup_Stop(mClipChannelGroup);

    for (s32 i = 0; i < mNumClipChannels; i++) {
        mClipChannelPool[i] = nullptr;
    }
}

void AudioPlayer::stopAll() {
    stopMusic();
    stopClips();
}

void AudioPlayer::setMusicVolume(f32 volume) {
    // currently only 1 concurrent music track is supported, so I can directly set the volume
    // if I want to do multiple music tracks at the same time in the future, then I'll need to do what setSfxVolume does to preserve relative volumes

    if (volume == 0.0f) {
        setIsMusicMuted(true);
        return;
    } else if (mIsMusicMuted) {
        setIsMusicMuted(false);
    }

    mMusicVolume = volume;
    volume *= mMasterVolume;

    if (mMusicChannel != nullptr && mIsPlayingMusic) {
        FMOD_Channel_SetVolume(mMusicChannel, volume);
    }
}

void AudioPlayer::setSfxVolume(f32 volume) {
    // should prevent divide by zero
    if (volume == 0.0f) {
        // just mute
        setIsSfxMuted(true);
        return;
    } else if (mIsSfxMuted && mMasterVolume > 0.0f) {
        setIsSfxMuted(false);
    }

    const f32 multiplier = volume / mSfxVolume;
    mSfxVolume = volume;

    // update volume of all channels
    // use the ratio of the old volume to the new one to make sure relative clip volumes are preserved
    for (s32 i = 0; i < mNumClipChannels; i++) {
        if (mClipChannelPool[i] != nullptr) {
            f32 currentVolume;
            auto result = FMOD_Channel_GetVolume(mClipChannelPool[i], &currentVolume);
            if (result == FMOD_OK) {
                FMOD_Channel_SetVolume(mClipChannelPool[i], currentVolume * multiplier);
            }
        }
    }
}

void AudioPlayer::setMasterVolume(f32 volume) {
    // should prevent divide by zero
    if (volume == 0.0f) {
        // just mute
        setIsMuted(true);
        return;
    } else if (isMuted()) {
        setIsMuted(false);
    }

    const f32 multiplier = volume / mMasterVolume;
    mMasterVolume = volume;

    // update music volume
    setMusicVolume(mMusicVolume);

    // update sfx volume. Same method as setSfxVolume, adjusting by a multiplier

    // update volume of all channels
    // use the ratio of the old volume to the new one to make sure relative clip volumes are preserved
    for (s32 i = 0; i < mNumClipChannels; i++) {
        if (mClipChannelPool[i] != nullptr) {
            f32 currentVolume;
            auto result = FMOD_Channel_GetVolume(mClipChannelPool[i], &currentVolume);
            if (result == FMOD_OK) {
                FMOD_Channel_SetVolume(mClipChannelPool[i], currentVolume * multiplier);
            }
        }
    }
}

void AudioPlayer::setIsMuted(bool isMuted) {
    setIsMusicMuted(isMuted);
    setIsSfxMuted(isMuted);
}

void AudioPlayer::setIsMusicMuted(bool isMuted) {
    if (mIsMusicMuted == isMuted) {
        return;
    }

    mIsMusicMuted = isMuted;
    if (mMusicChannel != nullptr) {
        FMOD_Channel_SetMute(mMusicChannel, isMuted);
    }
}

void AudioPlayer::setIsSfxMuted(bool isMuted) {
    if (mIsSfxMuted == isMuted) {
        return;
    }
    mIsSfxMuted = isMuted;

    for (s32 i = 0; i < mNumClipChannels; i++) {
        if (mClipChannelPool[i] != nullptr) {
            FMOD_Channel_SetMute(mClipChannelPool[i], isMuted);
        }
    }
}

void AudioPlayer::setListenerPosition(Vector2i worldPosition) {
    FMOD_VECTOR position = FMOD_VECTOR(worldPosition.x, worldPosition.y, 0);
    auto result = FMOD_System_Set3DListenerAttributes(mSystem, 0, &position, nullptr, nullptr, nullptr);
    if (result != FMOD_OK) {
        print("Error setting AudioListener position: ", FMOD_ErrorString(result));
    }
}

bool AudioPlayer::isMusicPaused() const {
    FMOD_BOOL isPaused = false;
    FMOD_Channel_GetPaused(mMusicChannel, &isPaused);
    return isPaused;
}

bool AudioPlayer::isClipsPaused() const {
    FMOD_BOOL isPaused = false;
    FMOD_ChannelGroup_GetPaused(mClipChannelGroup, &isPaused);
    return isPaused;
}

void AudioPlayer::pauseMusic(bool pause) {
    FMOD_Channel_SetPaused(mMusicChannel, pause);
}

void AudioPlayer::pauseClips(bool pause) {
    FMOD_ChannelGroup_SetPaused(mClipChannelGroup, pause);
}

void AudioPlayer::pauseAll(bool pause) {
    pauseMusic(pause);
    pauseClips(pause);
}

void AudioPlayer::setFilterMusic(Filter filter) {
    setChannelFilter(filter, mMusicChannel);
    mMusicFilter = filter;
}

void AudioPlayer::setFilterClips(Filter filter) {
    setChannelFilter(filter, mClipChannelGroup);
    mClipsFilter = filter;
}

// needs to be free'd with dsp->release();
Expected<FMOD_DSP*> AudioPlayer::createLowPassFilter(f32 cutoff, f32 resonance) {
    FMOD_DSP* dsp;
    FMOD_RESULT result = FMOD_System_CreateDSPByType(mSystem, FMOD_DSP_TYPE_LOWPASS, &dsp);
    if (result != FMOD_OK) {
        auto err = FMOD_ErrorString(result);
        return Error(sprint("Got error creating DSP:", err));
    }

    result = FMOD_DSP_SetParameterFloat(dsp, FMOD_DSP_LOWPASS_CUTOFF, cutoff);
    if (result != FMOD_OK) {
        return Error(sprint("Got error assigning lowpass cutoff:", FMOD_ErrorString(result)));
    }

    result = FMOD_DSP_SetParameterFloat(dsp, FMOD_DSP_LOWPASS_RESONANCE, resonance);
    if (result != FMOD_OK) {
        return Error(sprint("Got error assigning lowpass resonance:", FMOD_ErrorString(result)));
    }

    return dsp;
}

FMOD_DSP* AudioPlayer::getFilter(Filter filterType) {
    FMOD_DSP* dsp = nullptr;
    // TODO according to https://qa.fmod.com/t/does-a-dsp-object-used-on-multiple-channels-have-to-be-instantiated-that-many-times/14680
    // if I want to use a DSP on multiple channels I need multiple instances of it. When a channel is finished playing it will auto-remove the DSP
    // from itself, but it won't free the DSP. So it seems like tracking DSPs is going to be a huge pain.
    switch (filterType) {
    case Filter::None:
        break;
    case Filter::LowPass:
        if (mLowpassFilter == nullptr) {
            Expected<FMOD_DSP*> eDSP = createLowPassFilter();
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

    return dsp;
}

void AudioPlayer::setChannelFilter(Filter filter, FMOD_CHANNEL* channel) {
    FMOD_DSP* dsp = getFilter(filter);
    if (dsp == nullptr) {
        clearDSPs(channel);
    } else {
        FMOD_Channel_AddDSP(channel, FMOD_CHANNELCONTROL_DSP_TAIL, dsp);
    }
}

void AudioPlayer::setChannelFilter(Filter filter, FMOD_CHANNELGROUP* channelGroup) {
    FMOD_DSP* dsp = getFilter(filter);
    if (dsp == nullptr) {
        clearDSPs(channelGroup);
    } else {
        FMOD_ChannelGroup_AddDSP(channelGroup, FMOD_CHANNELCONTROL_DSP_TAIL, dsp);
    }
}

void AudioPlayer::clearDSPs(FMOD_CHANNEL* channel) {
    if (mLowpassFilter != nullptr) {
        FMOD_Channel_RemoveDSP(channel, mLowpassFilter);
    }
}

void AudioPlayer::clearDSPs(FMOD_CHANNELGROUP* channel) {
    if (mLowpassFilter != nullptr) {
        FMOD_ChannelGroup_RemoveDSP(channel, mLowpassFilter);
    }
}

void AudioPlayer::registerClip(const char* name, AudioClip clip) {
    assert(clip.isValid() && "Cannot register invalid clip");

    // not bothering to check for duplicates (skill issue)
    mClipRegistry.push_back({name, clip});
}

void AudioPlayer::unregisterClip(const char* name) {
    auto it = ecs::whal_find(mClipRegistry.begin(), mClipRegistry.end(), name);
    if (it == mClipRegistry.end()) {
        return;
    }
    mClipRegistry.erase(it);
}

AudioClip AudioPlayer::getClip(const char* name) {
    auto it = ecs::whal_find(mClipRegistry.begin(), mClipRegistry.end(), name);
    if (it == mClipRegistry.end()) {
#ifndef NDEBUG
        print("[AudioPlayer::getClip]: no clip with name", name, "found");
#endif
        return AudioClip();
    }

    return it->clip;
}

void AudioPlayer::clearClipRegistry() {
    for (auto& [name, clip] : mClipRegistry) {
        clip.unload();
    }

    mClipRegistry.clear();
}

}  // namespace whal

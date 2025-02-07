#pragma once

#include "CorradeOptional.h"

#include <vector>
#include "Util/Types.h"
#include "Util/Vector.h"
#include "whalECS/src/Expected.h"

#ifndef __EMSCRIPTEN__
namespace FMOD {

class System;
class Sound;
class Channel;
class ChannelGroup;
class ChannelControl;
class DSP;

}  // namespace FMOD
#else

#include <raylib.h>

#endif

namespace whal {

static constexpr s32 MAX_CHANNELS = 256;  // upper limit. just used for AudioPlayer's channel pool

struct System;
class AudioPlayer;

class AudioClip {
public:
    static Expected<AudioClip> from(const char* path);

    Corrade::Containers::Optional<Error> load(const char* path);
    void unload();

#ifndef __EMSCRIPTEN__
    bool isValid() const { return mSound != nullptr; }
    FMOD::Sound* get() const { return mSound; }
#else
    bool isValid() const { return mIsValid; }
    rl::Sound get() const { return mSound; }
#endif

private:
#ifndef __EMSCRIPTEN__
    FMOD::Sound* mSound = nullptr;
#else
    rl::Sound mSound;
    bool mIsValid = false;
#endif
};

class AudioPlayer {
public:
    enum class Filter { None, LowPass };

    friend System;
    friend AudioClip;

    AudioPlayer();
    Corrade::Containers::Optional<Error> init();
    void end();

    void playMusic(const char* path, f32 volume = 1.0, Filter filter = Filter::None, bool isLooping = true, Vector2i* position = nullptr);
    void playClip(const AudioClip& clip, f32 volume = 1.0, Filter filter = Filter::None, bool isLooping = false, Vector2i* position = nullptr);
    void playMenuClip(const AudioClip& clip, f32 volume = 1.0, Filter filter = Filter::None, bool isLooping = false);
    void playClip(const std::string& clipname, f32 volume = 1.0, Filter filter = Filter::None, bool isLooping = false, Vector2i* position = nullptr);
    void playMenuClip(const std::string& clipname, f32 volume = 1.0, Filter filter = Filter::None, bool isLooping = false);

    void stopMusic();
    void stopClips();
    void stopAll();
    bool isMusicPaused() const;
    bool isClipsPaused() const;
    void pauseMusic(bool pause);
    void pauseClips(bool pause);
    void pauseAll(bool pause);

    bool isValid() const { return mIsValid; }
    bool isMuted() const { return mIsMusicMuted && mIsSfxMuted; }
    bool isSfxMuted() const { return mIsSfxMuted; }
    bool isMusicMuted() const { return mIsMusicMuted; }
    void setIsMuted(bool isMuted);  // mutes music and sfx
    void setIsMusicMuted(bool isMuted);
    void setIsSfxMuted(bool isMuted);

    void setMusicVolume(f32 volume);
    void setSfxVolume(f32 volume);
    void setMasterVolume(f32 volume);
    f32 getMusicVolume() const { return mMusicVolume; }
    f32 getSfxVolume() const { return mSfxVolume; }
    f32 getMasterVolume() const { return mMasterVolume; }

    void update();

    void setFilterMusic(Filter filter);
    void setFilterClips(Filter filter);
    Filter getFilterMusic() const { return mMusicFilter; }
    Filter getFilterClips() const { return mClipsFilter; }

    void setListenerPosition(Vector2i worldPosition);

    // AudioClip Registry methods:
    // AudioClips can be registered with the AudioPlayer and played with "AudioPlayer.play(name)"

    // clip should be loaded
    void registerClip(const char* name, AudioClip clip);
    void unregisterClip(const char* name);

    // returns matching clip if name found in registry, or an invalid clip if none is found
    AudioClip getClip(const char* name);

    // unloads all clips in registry
    void clearClipRegistry();

private:
    struct RegisteredClip {
        std::string name;
        AudioClip clip;

        bool operator==(const RegisteredClip& other) const { return name == other.name; }
        bool operator==(const std::string& other) const { return name == other; }
    };

    AudioPlayer(const AudioPlayer&) = delete;
    void operator=(const AudioPlayer&) = delete;

#ifndef __EMSCRIPTEN__
    FMOD::System* getSystem() const;
    void playClipWithChannel(const AudioClip& clip, FMOD::Channel* channel, f32 volume, Filter filter, bool isLooping, Vector2i* position,
                             bool isInGroup = true);

    void setChannelFilter(Filter filter, FMOD::ChannelControl* channel);
    Expected<FMOD::DSP*> createLowPassFilter(f32 cutoff = 500, f32 resonance = 1);
    FMOD::Sound* mMusic = nullptr;
    FMOD::ChannelGroup* mClipChannelGroup = nullptr;
    FMOD::Channel* mClipChannelPool[MAX_CHANNELS];
    FMOD::Channel* mMusicChannel = nullptr;
    FMOD::Channel* mMenuChannel = nullptr;
    FMOD::System* mSystem = nullptr;
    FMOD::DSP* mLowpassFilter = nullptr;
#else
    rl::Music mMusic;
    std::vector<rl::Sound> mClipSounds;
    std::vector<rl::Sound> mMenuSounds;
    bool mIsClipsPaused = false;
#endif
    std::vector<RegisteredClip> mClipRegistry;
    s32 mMaxChannelCount = 0;
    s32 mNumMiscChannels = 2;  // MAKE SURE TO UPDATE THIS WITH MANUALLY MANAGED CHANNELS
    s32 mNumClipChannels = 0;
    f32 mMusicVolume = 1.0f;
    f32 mSfxVolume = 1.0f;
    f32 mMasterVolume = 1.0f;
    Filter mMusicFilter = Filter::None;
    Filter mClipsFilter = Filter::None;
    bool mIsValid = false;
    bool mIsPlayingMusic = false;
    bool mIsPlayingChannels = false;
    bool mIsMusicMuted = false;
    bool mIsSfxMuted = false;
};

}  // namespace whal

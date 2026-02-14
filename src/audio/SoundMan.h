#pragma once

#include "util/Singleton.h"
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// Forward declarations
struct SDL_AudioStream;

namespace ms
{

class WzProperty;

/**
 * @brief Sound effect cache item
 *
 * Based on SECACHEITEM from CSoundMan (IDA 0x5110b0).
 * Used for caching frequently played sound effects.
 */
struct SECacheItem
{
    std::uint32_t nLastPlayTime{0};  // Time when last played (for LRU eviction)
    std::string sPath;                // Sound path
    std::vector<std::uint8_t> data;   // Cached audio data

    bool operator<(const SECacheItem& other) const
    {
        return nLastPlayTime < other.nLastPlayTime;
    }
};

/**
 * @brief Ambient sound state
 *
 * Based on AmbientSound from CSoundMan (IDA 0xfb97a0).
 * Used for looping ambient sounds with volume control.
 */
struct AmbientSound
{
    std::uint32_t nCookie{0};         // Unique identifier
    std::uint32_t nRate{100};         // Volume rate (0-200)
    std::uint32_t nVolume{0};         // Current volume
    std::string sPath;                // Sound path
    bool bPlaying{false};             // Is currently playing
};

/**
 * @brief Decoded audio data
 *
 * PCM audio data decoded from MP3.
 */
struct DecodedAudio
{
    std::vector<std::int16_t> samples; // PCM samples (16-bit signed)
    int channels{0};                   // Number of channels (1 or 2)
    int sampleRate{0};                 // Sample rate in Hz
};

/**
 * @brief Fade effect type
 */
enum class FadeType
{
    None,
    FadeIn,
    FadeOut
};

/**
 * @brief Sound state for active sounds
 *
 * Based on IWzSoundState interface from original client.
 * Tracks playback state of individual sounds.
 */
struct SoundState
{
    std::uint32_t nCookie{0};         // Unique identifier
    std::int32_t nVolume{100};        // Current volume (0-128)
    std::int32_t nTargetVolume{100};  // Target volume for fade (0-128)
    bool bPlaying{false};             // Is currently playing
    bool bLooping{false};             // Is looping
    std::string sPath;                // Sound path
    SDL_AudioStream* pAudioStream{nullptr}; // SDL audio stream
    DecodedAudio decodedAudio;        // Decoded PCM data for looping
    std::size_t playbackPos{0};       // Current playback position for looping

    // Fade effect state
    FadeType fadeType{FadeType::None};
    std::uint32_t nFadeStartTime{0};   // Time when fade started
    std::uint32_t nFadeDuration{0};    // Fade duration in ms
    std::int32_t nFadeStartVolume{0};  // Volume at fade start
    std::int32_t nFadeEndVolume{0};    // Volume at fade end
    bool bStopAfterFade{false};        // Stop sound after fade completes
};

/**
 * @brief Sound system manager
 *
 * Based on CSoundMan from MapleStory v1029 client.
 * Singleton class for managing all audio playback.
 *
 * Original CSoundMan (from IDA):
 * - Constructor: CSoundMan() @ 0x1a925f0
 * - Destructor: ~CSoundMan() @ 0x1a926e0
 * - Init(HWND, uint, long, long, long) @ 0xfb8a90
 * - Term() @ 0xfb91f0
 * - PlayBGM(wchar_t*, long, uint, uint, long, uint) @ 0x56d410
 * - PlaySE(wchar_t*, uint, long, long, uint) @ 0x573200
 * - PlayAmbient(wchar_t*, uint, uint) @ 0xfb97a0
 * - StopAmbient(uint, uint) @ 0xfb9880
 * - SetBGMVolume(uint, uint) @ 0xfb9670
 * - SetSEVolume(uint) @ 0xfb9aa0
 * - StopSE(uint, uint) @ 0xfb9da0
 * - FlushSECache(uint) @ 0x5183a0
 *
 * Member variables (from constructor decompilation):
 * - m_uBGMVolume: BGM volume (default 100)
 * - m_uSEVolume: SE volume (default 100)
 * - m_pBGMState: Current BGM sound state
 * - m_mAmbientSound: Map of ambient sounds
 * - m_heapSECache: Heap for SE cache (LRU eviction)
 * - m_mposSECache: Map for SE cache positions
 * - m_mpLoopingStates: Map of looping sound states
 */
class SoundMan final : public Singleton<SoundMan>
{
    friend class Singleton<SoundMan>;

public:
    /**
     * @brief Initialize sound system
     *
     * Based on CSoundMan::Init (0xfb8a90).
     * Original uses StringPool idx 0xB11, 0xB12 for resource paths.
     *
     * @param nChannels Number of audio channels (default 32)
     * @param nSampleRate Sample rate (default 44100)
     * @param nBitsPerSample Bits per sample (default 16)
     * @return True if initialization succeeded
     */
    [[nodiscard]] auto Initialize(std::uint32_t nChannels = 32,
                                   std::uint32_t nSampleRate = 44100,
                                   std::uint32_t nBitsPerSample = 16) -> bool;

    /**
     * @brief Terminate sound system
     *
     * Based on CSoundMan::Term (0xfb91f0).
     */
    void Shutdown();

    // ========== BGM (Background Music) ==========

    /**
     * @brief Play background music
     *
     * Based on CSoundMan::PlayBGM (0x56d410).
     * Handles looping, fade in/out, volume control.
     *
     * @param sPath WZ path to music (e.g., "Sound/Bgm0.img/NightMarket")
     * @param nLoop Loop count (-1 for infinite, 0 for none)
     * @param nStartVolume128 Start volume (0-128, scaled by system volume)
     * @param nEndVolume128 End volume for fade (0-128)
     * @param nFadeInTime Fade in time in ms
     * @param nFadeOutTime Fade out time in ms
     */
    void PlayBGM(const std::string& sPath,
                 std::int32_t nLoop = -1,
                 std::uint32_t nStartVolume128 = 128,
                 std::uint32_t nEndVolume128 = 128,
                 std::int32_t nFadeInTime = 0,
                 std::uint32_t nFadeOutTime = 0);

    /**
     * @brief Stop background music
     *
     * @param nFadeOutTime Fade out time in ms (0 for immediate)
     */
    void StopBGM(std::uint32_t nFadeOutTime = 0);

    /**
     * @brief Set BGM volume
     *
     * Based on CSoundMan::SetBGMVolume (0xfb9670).
     * Also updates ambient sound volumes.
     *
     * @param nVolume Volume level (0-100)
     * @param nFadingDuration Fade duration in ms
     */
    void SetBGMVolume(std::uint32_t nVolume, std::uint32_t nFadingDuration = 0);

    /**
     * @brief Get BGM volume
     *
     * Based on CSoundMan::GetBGMVolume (0x9952a0).
     */
    [[nodiscard]] auto GetBGMVolume() const noexcept -> std::uint32_t { return m_uBGMVolume; }

    /**
     * @brief Get current BGM position in milliseconds
     */
    [[nodiscard]] auto GetBGMPosition() const -> std::uint32_t;

    /**
     * @brief Set BGM position in milliseconds
     * @param nMs Position in milliseconds
     */
    void SetBGMPosition(std::uint32_t nMs);

    // ========== Sound Effects (SE) ==========

    /**
     * @brief Play sound effect
     *
     * Based on CSoundMan::PlaySE (0x573200).
     * SE caching with heap-based LRU eviction.
     * Volume calculation: (nStartVolume128 * m_uSEVolume / 100)
     *
     * @param sPath WZ path to sound
     * @param nStartVolume128 Start volume (0-128)
     * @param nLoop Loop count (-1 for infinite, default 0)
     * @param nPan Pan position (-128 to 128, 0 = center)
     * @param nFadeOutTime Fade out time in ms
     * @return Cookie for tracking/stopping the sound
     */
    auto PlaySE(const std::string& sPath,
                std::uint32_t nStartVolume128 = 128,
                std::int32_t nLoop = 0,
                std::int32_t nPan = 0,
                std::uint32_t nFadeOutTime = 0) -> std::uint32_t;

    /**
     * @brief Play safe sound effect (with validation)
     *
     * Based on CSoundMan::PlaySafeSE (0x51a9c0).
     */
    auto PlaySafeSE(const std::string& sPath,
                    std::int32_t& rCookie,
                    std::uint32_t nStartVolume128 = 128,
                    std::int32_t nLoop = 0) -> std::uint32_t;

    /**
     * @brief Stop sound effect
     *
     * Based on CSoundMan::StopSE (0xfb9da0).
     *
     * @param nCookie Cookie from PlaySE (0 to stop all)
     * @param nFadeOutTime Fade out time in ms
     */
    void StopSE(std::uint32_t nCookie = 0, std::uint32_t nFadeOutTime = 0);

    /**
     * @brief Set SE volume
     *
     * Based on CSoundMan::SetSEVolume (0xfb9aa0).
     *
     * @param nVolume Volume level (0-100)
     */
    void SetSEVolume(std::uint32_t nVolume);

    /**
     * @brief Get SE volume
     *
     * Based on CSoundMan::GetSEVolume (0x18a6c20).
     */
    [[nodiscard]] auto GetSEVolume() const noexcept -> std::uint32_t { return m_uSEVolume; }

    /**
     * @brief Flush SE cache
     *
     * Based on CSoundMan::FlushSECache (0x5183a0).
     * Removes oldest entries from cache.
     *
     * @param nCount Number of entries to flush
     */
    void FlushSECache(std::uint32_t nCount);

    // ========== Ambient Sounds ==========

    /**
     * @brief Play ambient sound
     *
     * Based on CSoundMan::PlayAmbient (0xfb97a0).
     * Ambient sounds are looping background sounds with volume control.
     *
     * @param sPath WZ path to sound
     * @param nVolumeRate Volume rate (0-200, percentage)
     * @param nFadeInTime Fade in time in ms
     * @return Cookie for tracking/stopping
     */
    auto PlayAmbient(const std::string& sPath,
                     std::uint32_t nVolumeRate = 100,
                     std::uint32_t nFadeInTime = 0) -> std::uint32_t;

    /**
     * @brief Stop ambient sound
     *
     * Based on CSoundMan::StopAmbient (0xfb9880).
     *
     * @param nCookie Cookie from PlayAmbient (0 to stop all)
     * @param nFadeOutTime Fade out time in ms
     */
    void StopAmbient(std::uint32_t nCookie = 0, std::uint32_t nFadeOutTime = 0);

    /**
     * @brief Check if sound is ambient
     *
     * Based on CSoundMan::IsAmbientSound (0xfb99e0).
     */
    [[nodiscard]] auto IsAmbientSound(std::uint32_t nCookie) const -> bool;

    // ========== Exclusive Sound Effects ==========

    /**
     * @brief Play exclusive sound effect (only one at a time)
     *
     * Based on CSoundMan::PlayExclSE (0x949e20).
     */
    void PlayExclSE(const std::string& sPath, std::uint32_t nVolume128, bool bLoop = false);

    /**
     * @brief Stop exclusive sound effect
     *
     * Based on CSoundMan::StopExclSE (0xfb7f50).
     */
    void StopExclSE();

    /**
     * @brief Check if exclusive SE is playing
     *
     * Based on CSoundMan::IsExclSEPlaying (0xfb7fd0).
     */
    [[nodiscard]] auto IsExclSEPlaying() const -> bool;

    // ========== Skill Voice ==========

    /**
     * @brief Play skill voice
     *
     * Based on CSoundMan::PlaySkillVoice (0x19d3de0).
     */
    void PlaySkillVoice(const std::string& sPath, std::uint32_t nVolume128, bool bLoop = false);

    /**
     * @brief Stop skill voice
     *
     * Based on CSoundMan::StopSkillVoice (0xfb83b0).
     */
    void StopSkillVoice();

    /**
     * @brief Check if skill voice is playing
     *
     * Based on CSoundMan::IsSkillVoicePlaying (0xfb8080).
     */
    [[nodiscard]] auto IsSkillVoicePlaying() const -> bool;

    /**
     * @brief Set skill voice volume
     *
     * Based on CSoundMan::SetSkillVoiceVolume (0xfb8030).
     */
    void SetSkillVoiceVolume(std::uint32_t nVolume);

    // ========== State ==========

    [[nodiscard]] auto IsInitialized() const noexcept -> bool { return m_bInitialized; }

    /**
     * @brief Update sound system (call each frame)
     */
    void Update();

private:
    SoundMan();
    ~SoundMan() override;

    /**
     * @brief Load sound data from WZ
     * @param sPath WZ path
     * @return Audio data, or empty vector on failure
     */
    [[nodiscard]] auto LoadSoundFromWZ(const std::string& sPath) -> std::vector<std::uint8_t>;

    /**
     * @brief Decode MP3 data to PCM
     * @param mp3Data Raw MP3 bytes
     * @return Decoded audio, or empty on failure
     */
    [[nodiscard]] auto DecodeMP3(const std::vector<std::uint8_t>& mp3Data) -> DecodedAudio;

    /**
     * @brief Create SDL audio stream and play audio
     * @param audio Decoded audio data
     * @param volume Volume (0-128)
     * @param loop Whether to loop
     * @return SDL audio stream, or nullptr on failure
     */
    [[nodiscard]] auto PlayAudio(const DecodedAudio& audio, int volume, bool loop) -> SDL_AudioStream*;

    /**
     * @brief Stop and destroy an audio stream
     * @param stream Audio stream to destroy
     */
    void DestroyAudioStream(SDL_AudioStream* stream);

    /**
     * @brief Get or create cached SE
     * @param sPath WZ path
     * @return Pointer to cache item, or nullptr
     */
    [[nodiscard]] auto GetOrCreateCachedSE(const std::string& sPath) -> SECacheItem*;

    /**
     * @brief Update fade effects for all active sounds
     * Called by Update() to process ongoing fade in/out
     */
    void UpdateFadeEffects();

    // ========== Member Variables (from CSoundMan constructor 0x1a925f0) ==========

    // Volume settings (0-100)
    std::uint32_t m_uBGMVolume{100};   // BGM volume
    std::uint32_t m_uSEVolume{100};    // SE volume
    std::uint32_t m_uVoiceVolume{100}; // Voice volume

    // Cookie counters for unique identifiers
    std::uint32_t m_uSESerial{0};       // SE cookie counter
    std::uint32_t m_uAmbientSerial{0};  // Ambient cookie counter

    // BGM state
    std::unique_ptr<SoundState> m_pBGMState;
    std::string m_sBGMPath;

    // Exclusive SE state
    std::unique_ptr<SoundState> m_pExclSEState;

    // Voice state
    std::unique_ptr<SoundState> m_pVoiceState;

    // Ambient sounds map (cookie -> AmbientSound)
    std::map<std::uint32_t, AmbientSound> m_mAmbientSound;

    // SE cache (LRU eviction based on nLastPlayTime)
    std::vector<SECacheItem> m_heapSECache;
    std::unordered_map<std::string, std::size_t> m_mposSECache;  // path -> cache index
    static constexpr std::size_t MAX_SE_CACHE_SIZE = 64;

    // Looping sound states (cookie -> SoundState)
    std::map<std::uint32_t, std::unique_ptr<SoundState>> m_mpLoopingStates;

    // Initialization state
    bool m_bInitialized{false};

    // Audio parameters
    std::uint32_t m_nChannels{32};
    std::uint32_t m_nSampleRate{44100};
    std::uint32_t m_nBitsPerSample{16};
};

} // namespace ms

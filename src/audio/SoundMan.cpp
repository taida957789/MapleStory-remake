#include "SoundMan.h"
#include "util/Logger.h"
#include "wz/WzProperty.h"
#include "wz/WzResMan.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cstring>

// minimp3 - single-header MP3 decoder
#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"

namespace ms
{

SoundMan::SoundMan() = default;

SoundMan::~SoundMan()
{
    Shutdown();
}

auto SoundMan::Initialize(std::uint32_t nChannels,
                              std::uint32_t nSampleRate,
                              std::uint32_t nBitsPerSample) -> bool
{
    // Based on CSoundMan::Init (0xfb8a90)
    // Original uses IWzSound::raw_GlobalInit with channels/sampleRate/bitsPerSample

    if (m_bInitialized)
    {
        return true;
    }

    m_nChannels = nChannels;
    m_nSampleRate = nSampleRate;
    m_nBitsPerSample = nBitsPerSample;

    // Initialize SDL audio subsystem
    if (!SDL_WasInit(SDL_INIT_AUDIO))
    {
        if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
        {
            LOG_ERROR("Failed to initialize SDL audio: {}", SDL_GetError());
            return false;
        }
    }

    // Initialize state
    m_uBGMVolume = 100;
    m_uSEVolume = 100;
    m_uVoiceVolume = 100;
    m_uSESerial = 0;
    m_uAmbientSerial = 0;

    m_pBGMState.reset();
    m_pExclSEState.reset();
    m_pVoiceState.reset();
    m_mAmbientSound.clear();
    m_heapSECache.clear();
    m_mposSECache.clear();
    m_mpLoopingStates.clear();

    m_bInitialized = true;
    LOG_INFO("Sound system initialized (channels={}, sampleRate={}, bitsPerSample={})",
             nChannels, nSampleRate, nBitsPerSample);

    return true;
}

void SoundMan::Shutdown()
{
    // Based on CSoundMan::Term (0xfb91f0)

    if (!m_bInitialized)
    {
        return;
    }

    // Stop all sounds (this will clean up audio streams)
    StopBGM(0);
    StopSE(0, 0);
    StopAmbient(0, 0);
    StopExclSE();
    StopSkillVoice();

    // Clear all state
    m_pBGMState.reset();
    m_pExclSEState.reset();
    m_pVoiceState.reset();
    m_mAmbientSound.clear();
    m_heapSECache.clear();
    m_mposSECache.clear();
    m_mpLoopingStates.clear();

    // Note: SDL_QuitSubSystem(SDL_INIT_AUDIO) is handled by Application
    m_bInitialized = false;
    LOG_INFO("Sound system shutdown");
}

// ========== BGM ==========

void SoundMan::PlayBGM(const std::string& sPath,
                           std::int32_t nLoop,
                           std::uint32_t nStartVolume128,
                           std::uint32_t nEndVolume128,
                           std::int32_t nFadeInTime,
                           std::uint32_t nFadeOutTime)
{
    // Based on CSoundMan::PlayBGM (0x56d410)
    // Original loads via IWzResMan::GetObjectA, handles looping, fade in/out

    if (!m_bInitialized)
    {
        return;
    }

    // Stop current BGM if playing
    if (m_pBGMState && m_pBGMState->bPlaying)
    {
        StopBGM(nFadeOutTime);
    }

    // Calculate volume: (nStartVolume128 * m_uBGMVolume / 100)
    std::uint32_t actualVolume = nStartVolume128 * m_uBGMVolume / 100;

    // Load MP3 data from WZ
    auto mp3Data = LoadSoundFromWZ(sPath);
    if (mp3Data.empty())
    {
        LOG_WARN("Failed to load BGM: {}", sPath);
        return;
    }

    // Decode MP3
    auto decodedAudio = DecodeMP3(mp3Data);
    if (decodedAudio.samples.empty())
    {
        LOG_WARN("Failed to decode BGM: {}", sPath);
        return;
    }

    // Create BGM state
    m_pBGMState = std::make_unique<SoundState>();
    m_pBGMState->sPath = sPath;
    m_pBGMState->nTargetVolume = static_cast<std::int32_t>(nEndVolume128 * m_uBGMVolume / 100);
    m_pBGMState->bLooping = (nLoop != 0);
    m_pBGMState->bPlaying = true;
    m_pBGMState->decodedAudio = std::move(decodedAudio);
    m_sBGMPath = sPath;

    // Set up fade in if requested
    if (nFadeInTime > 0)
    {
        // Start at 0 volume and fade up to actualVolume
        m_pBGMState->nVolume = 0;
        m_pBGMState->fadeType = FadeType::FadeIn;
        m_pBGMState->nFadeStartTime = static_cast<std::uint32_t>(SDL_GetTicks());
        m_pBGMState->nFadeDuration = static_cast<std::uint32_t>(nFadeInTime);
        m_pBGMState->nFadeStartVolume = 0;
        m_pBGMState->nFadeEndVolume = static_cast<std::int32_t>(actualVolume);
    }
    else
    {
        m_pBGMState->nVolume = static_cast<std::int32_t>(actualVolume);
    }

    // Play audio (start at current volume, fade will adjust it)
    m_pBGMState->pAudioStream = PlayAudio(m_pBGMState->decodedAudio,
                                           m_pBGMState->nVolume,
                                           m_pBGMState->bLooping);

    if (!m_pBGMState->pAudioStream)
    {
        LOG_WARN("Failed to create BGM audio stream: {}", sPath);
        m_pBGMState->bPlaying = false;
    }
}

void SoundMan::StopBGM(std::uint32_t nFadeOutTime)
{
    if (!m_pBGMState)
    {
        return;
    }

    LOG_DEBUG("StopBGM: {} (fadeOut={})", m_sBGMPath, nFadeOutTime);

    if (nFadeOutTime > 0)
    {
        // Start fade out - actual stop will happen in UpdateFadeEffects
        m_pBGMState->fadeType = FadeType::FadeOut;
        m_pBGMState->nFadeStartTime = static_cast<std::uint32_t>(SDL_GetTicks());
        m_pBGMState->nFadeDuration = nFadeOutTime;
        m_pBGMState->nFadeStartVolume = m_pBGMState->nVolume;
        m_pBGMState->nFadeEndVolume = 0;
        m_pBGMState->bStopAfterFade = true;
    }
    else
    {
        // Immediate stop
        if (m_pBGMState->pAudioStream)
        {
            DestroyAudioStream(m_pBGMState->pAudioStream);
            m_pBGMState->pAudioStream = nullptr;
        }

        m_pBGMState->bPlaying = false;
        m_pBGMState.reset();
        m_sBGMPath.clear();
    }
}

void SoundMan::SetBGMVolume(std::uint32_t nVolume, std::uint32_t nFadingDuration)
{
    // Based on CSoundMan::SetBGMVolume (0xfb9670)
    // Also updates ambient sound volumes

    std::uint32_t oldVolume = m_uBGMVolume;
    m_uBGMVolume = std::min(nVolume, 100u);

    LOG_DEBUG("SetBGMVolume: {} -> {} (fade={})", oldVolume, m_uBGMVolume, nFadingDuration);

    // Update BGM state volume
    if (m_pBGMState && m_pBGMState->bPlaying)
    {
        std::int32_t volumeDelta = static_cast<std::int32_t>(m_uBGMVolume) -
                                    static_cast<std::int32_t>(oldVolume);
        m_pBGMState->nVolume += volumeDelta;
        m_pBGMState->nVolume = std::clamp(m_pBGMState->nVolume, 0, 128);
    }

    // Update ambient sound volumes
    for (auto& [cookie, ambient] : m_mAmbientSound)
    {
        std::uint32_t newVolume = m_uBGMVolume * ambient.nRate / 100;
        ambient.nVolume = newVolume;
    }
}

auto SoundMan::GetBGMPosition() const -> std::uint32_t
{
    // TODO: Actual position tracking
    return 0;
}

void SoundMan::SetBGMPosition(std::uint32_t nMs)
{
    // TODO: Actual seek implementation
    (void)nMs;
}

// ========== Sound Effects ==========

auto SoundMan::PlaySE(const std::string& sPath,
                          std::uint32_t nStartVolume128,
                          std::int32_t nLoop,
                          std::int32_t nPan,
                          std::uint32_t nFadeOutTime) -> std::uint32_t
{
    // Based on CSoundMan::PlaySE (0x573200)
    // SE caching with heap-based LRU eviction
    // Volume calculation: (nStartVolume128 * m_uSEVolume / 100)

    if (!m_bInitialized)
    {
        return 0;
    }

    // Generate unique cookie
    ++m_uSESerial;
    std::uint32_t cookie = m_uSESerial;

    // Calculate volume
    std::uint32_t actualVolume = nStartVolume128 * m_uSEVolume / 100;

    // Try to get from cache
    auto* cacheItem = GetOrCreateCachedSE(sPath);
    if (!cacheItem || cacheItem->data.empty())
    {
        LOG_DEBUG("SE not cached or failed to load: {}", sPath);
        return 0;
    }

    // Decode MP3 from cache
    auto decodedAudio = DecodeMP3(cacheItem->data);
    if (decodedAudio.samples.empty())
    {
        LOG_WARN("Failed to decode SE: {}", sPath);
        return 0;
    }

    // Track looping sounds
    bool isLooping = (nLoop != 0);
    if (isLooping)
    {
        auto state = std::make_unique<SoundState>();
        state->nCookie = cookie;
        state->sPath = sPath;
        state->nVolume = static_cast<std::int32_t>(actualVolume);
        state->bLooping = true;
        state->bPlaying = true;
        state->decodedAudio = std::move(decodedAudio);
        state->pAudioStream = PlayAudio(state->decodedAudio,
                                         static_cast<int>(actualVolume),
                                         true);
        m_mpLoopingStates[cookie] = std::move(state);
    }
    else
    {
        // For one-shot SE, just play it (no tracking needed for non-looping)
        auto* stream = PlayAudio(decodedAudio, static_cast<int>(actualVolume), false);
        // One-shot sounds will be automatically cleaned up by SDL when done
        (void)stream;
    }

    // TODO: Implement pan using nPan
    (void)nPan;
    (void)nFadeOutTime;

    return cookie;
}

auto SoundMan::PlaySafeSE(const std::string& sPath,
                              std::int32_t& rCookie,
                              std::uint32_t nStartVolume128,
                              std::int32_t nLoop) -> std::uint32_t
{
    // Based on CSoundMan::PlaySafeSE (0x51a9c0)
    // Validates and plays SE, returning cookie

    if (sPath.empty())
    {
        rCookie = 0;
        return 0;
    }

    auto cookie = PlaySE(sPath, nStartVolume128, nLoop, 0, 0);
    rCookie = static_cast<std::int32_t>(cookie);
    return cookie;
}

auto SoundMan::PlayFieldSound(const std::string& sPath,
                              std::uint32_t nVolume128) -> std::uint32_t
{
    return PlaySE(sPath, nVolume128);
}

void SoundMan::StopSE(std::uint32_t nCookie, std::uint32_t nFadeOutTime)
{
    // Based on CSoundMan::StopSE (0xfb9da0)

    if (nCookie == 0)
    {
        // Stop all SE
        LOG_DEBUG("StopSE: all (fadeOut={})", nFadeOutTime);
        for (auto& [_, state] : m_mpLoopingStates)
        {
            if (state && state->pAudioStream)
            {
                DestroyAudioStream(state->pAudioStream);
                state->pAudioStream = nullptr;
            }
        }
        m_mpLoopingStates.clear();
    }
    else
    {
        // Stop specific SE
        LOG_DEBUG("StopSE: cookie={} (fadeOut={})", nCookie, nFadeOutTime);
        auto it = m_mpLoopingStates.find(nCookie);
        if (it != m_mpLoopingStates.end())
        {
            if (it->second && it->second->pAudioStream)
            {
                DestroyAudioStream(it->second->pAudioStream);
            }
            m_mpLoopingStates.erase(it);
        }
    }

    // TODO: Implement fade out using nFadeOutTime
    (void)nFadeOutTime;
}

void SoundMan::SetSEVolume(std::uint32_t nVolume)
{
    // Based on CSoundMan::SetSEVolume (0xfb9aa0)

    std::uint32_t oldVolume = m_uSEVolume;
    m_uSEVolume = std::min(nVolume, 100u);

    LOG_DEBUG("SetSEVolume: {} -> {}", oldVolume, m_uSEVolume);

    // Update all active SE volumes (except BGM and ambient)
    if (oldVolume > 0)
    {
        for (auto& [cookie, state] : m_mpLoopingStates)
        {
            std::int32_t newVolume = state->nVolume * static_cast<std::int32_t>(m_uSEVolume) /
                                      static_cast<std::int32_t>(oldVolume);
            state->nVolume = newVolume;
        }
    }
}

void SoundMan::FlushSECache(std::uint32_t nCount)
{
    // Based on CSoundMan::FlushSECache (0x5183a0)
    // Removes oldest entries from cache using min-heap

    if (m_heapSECache.empty() || nCount == 0)
    {
        return;
    }

    // Sort by last play time (oldest first)
    std::sort(m_heapSECache.begin(), m_heapSECache.end());

    // Remove oldest entries
    std::uint32_t toRemove = std::min(nCount, static_cast<std::uint32_t>(m_heapSECache.size()));
    for (std::uint32_t i = 0; i < toRemove; ++i)
    {
        const auto& item = m_heapSECache[i];
        m_mposSECache.erase(item.sPath);
    }

    m_heapSECache.erase(m_heapSECache.begin(),
                        m_heapSECache.begin() + static_cast<std::ptrdiff_t>(toRemove));

    // Rebuild index
    m_mposSECache.clear();
    for (std::size_t i = 0; i < m_heapSECache.size(); ++i)
    {
        m_mposSECache[m_heapSECache[i].sPath] = i;
    }

    LOG_DEBUG("FlushSECache: removed {} entries, {} remaining", toRemove, m_heapSECache.size());
}

// ========== Ambient Sounds ==========

auto SoundMan::PlayAmbient(const std::string& sPath,
                               std::uint32_t nVolumeRate,
                               std::uint32_t nFadeInTime) -> std::uint32_t
{
    // Based on CSoundMan::PlayAmbient (0xfb97a0)

    if (!m_bInitialized)
    {
        return 0;
    }

    // Clamp volume rate to 0-200
    nVolumeRate = std::min(nVolumeRate, 200u);

    // Generate unique cookie
    ++m_uAmbientSerial;
    std::uint32_t cookie = m_uAmbientSerial;

    // Calculate actual volume (scaled by BGM volume)
    std::uint32_t actualVolume = m_uBGMVolume * nVolumeRate / 100;
    // Convert to 0-128 range for audio playback
    std::uint32_t volume128 = actualVolume * 128 / 100;

    // Load and decode audio
    auto mp3Data = LoadSoundFromWZ(sPath);
    if (mp3Data.empty())
    {
        LOG_WARN("Failed to load ambient sound: {}", sPath);
        return 0;
    }

    auto decodedAudio = DecodeMP3(mp3Data);
    if (decodedAudio.samples.empty())
    {
        LOG_WARN("Failed to decode ambient sound: {}", sPath);
        return 0;
    }

    // Create ambient sound entry
    AmbientSound ambient;
    ambient.nCookie = cookie;
    ambient.sPath = sPath;
    ambient.nRate = nVolumeRate;
    ambient.nVolume = actualVolume;
    ambient.bPlaying = true;

    m_mAmbientSound[cookie] = ambient;

    // Play audio (ambient sounds are always looping)
    // Store in looping states for playback tracking
    auto state = std::make_unique<SoundState>();
    state->nCookie = cookie;
    state->sPath = sPath;
    state->nTargetVolume = static_cast<std::int32_t>(volume128);
    state->bLooping = true;
    state->bPlaying = true;
    state->decodedAudio = std::move(decodedAudio);

    // Set up fade in if requested
    if (nFadeInTime > 0)
    {
        state->nVolume = 0;
        state->fadeType = FadeType::FadeIn;
        state->nFadeStartTime = static_cast<std::uint32_t>(SDL_GetTicks());
        state->nFadeDuration = nFadeInTime;
        state->nFadeStartVolume = 0;
        state->nFadeEndVolume = static_cast<std::int32_t>(volume128);
    }
    else
    {
        state->nVolume = static_cast<std::int32_t>(volume128);
    }

    state->pAudioStream = PlayAudio(state->decodedAudio,
                                     state->nVolume,
                                     true);
    m_mpLoopingStates[cookie] = std::move(state);

    return cookie;
}

void SoundMan::StopAmbient(std::uint32_t nCookie, std::uint32_t nFadeOutTime)
{
    // Based on CSoundMan::StopAmbient (0xfb9880)

    // Helper lambda to start fade out or immediately stop
    auto stopWithFade = [this, nFadeOutTime](std::uint32_t cookie) {
        auto loopIt = m_mpLoopingStates.find(cookie);
        if (loopIt == m_mpLoopingStates.end() || !loopIt->second)
        {
            return;
        }

        if (nFadeOutTime > 0)
        {
            // Start fade out - actual stop will happen in UpdateFadeEffects
            loopIt->second->fadeType = FadeType::FadeOut;
            loopIt->second->nFadeStartTime = static_cast<std::uint32_t>(SDL_GetTicks());
            loopIt->second->nFadeDuration = nFadeOutTime;
            loopIt->second->nFadeStartVolume = loopIt->second->nVolume;
            loopIt->second->nFadeEndVolume = 0;
            loopIt->second->bStopAfterFade = true;
        }
        else
        {
            // Immediate stop
            if (loopIt->second->pAudioStream)
            {
                DestroyAudioStream(loopIt->second->pAudioStream);
            }
            m_mpLoopingStates.erase(loopIt);
            m_mAmbientSound.erase(cookie);
        }
    };

    if (nCookie == 0)
    {
        // Stop all ambient sounds
        LOG_DEBUG("StopAmbient: all (fadeOut={})", nFadeOutTime);
        std::vector<std::uint32_t> cookies;
        for (auto& [ambientCookie, ambient] : m_mAmbientSound)
        {
            cookies.push_back(ambientCookie);
        }
        for (auto c : cookies)
        {
            stopWithFade(c);
        }
        // If immediate stop, clear the map (fade will clear via UpdateFadeEffects)
        if (nFadeOutTime == 0)
        {
            m_mAmbientSound.clear();
        }
    }
    else
    {
        // Stop specific ambient sound
        auto it = m_mAmbientSound.find(nCookie);
        if (it != m_mAmbientSound.end())
        {
            LOG_DEBUG("StopAmbient: cookie={} path={} (fadeOut={})",
                      nCookie, it->second.sPath, nFadeOutTime);
            stopWithFade(nCookie);
        }
    }
}

auto SoundMan::IsAmbientSound(std::uint32_t nCookie) const -> bool
{
    // Based on CSoundMan::IsAmbientSound (0xfb99e0)
    return m_mAmbientSound.find(nCookie) != m_mAmbientSound.end();
}

// ========== Exclusive Sound Effects ==========

void SoundMan::PlayExclSE(const std::string& sPath, std::uint32_t nVolume128, bool bLoop)
{
    // Based on CSoundMan::PlayExclSE (0x949e20)

    if (!m_bInitialized)
    {
        return;
    }

    // Stop current exclusive SE
    StopExclSE();

    std::uint32_t actualVolume = nVolume128 * m_uSEVolume / 100;

    // Load and decode audio
    auto mp3Data = LoadSoundFromWZ(sPath);
    if (mp3Data.empty())
    {
        LOG_WARN("Failed to load exclusive SE: {}", sPath);
        return;
    }

    auto decodedAudio = DecodeMP3(mp3Data);
    if (decodedAudio.samples.empty())
    {
        LOG_WARN("Failed to decode exclusive SE: {}", sPath);
        return;
    }

    m_pExclSEState = std::make_unique<SoundState>();
    m_pExclSEState->sPath = sPath;
    m_pExclSEState->nVolume = static_cast<std::int32_t>(actualVolume);
    m_pExclSEState->bLooping = bLoop;
    m_pExclSEState->bPlaying = true;
    m_pExclSEState->decodedAudio = std::move(decodedAudio);
    m_pExclSEState->pAudioStream = PlayAudio(m_pExclSEState->decodedAudio,
                                              static_cast<int>(actualVolume),
                                              bLoop);
}

void SoundMan::StopExclSE()
{
    // Based on CSoundMan::StopExclSE (0xfb7f50)

    if (m_pExclSEState)
    {
        LOG_DEBUG("StopExclSE: {}", m_pExclSEState->sPath);

        if (m_pExclSEState->pAudioStream)
        {
            DestroyAudioStream(m_pExclSEState->pAudioStream);
            m_pExclSEState->pAudioStream = nullptr;
        }

        m_pExclSEState->bPlaying = false;
        m_pExclSEState.reset();
    }
}

auto SoundMan::IsExclSEPlaying() const -> bool
{
    // Based on CSoundMan::IsExclSEPlaying (0xfb7fd0)
    return m_pExclSEState && m_pExclSEState->bPlaying;
}

// ========== Skill Voice ==========

void SoundMan::PlaySkillVoice(const std::string& sPath, std::uint32_t nVolume128, bool bLoop)
{
    // Based on CSoundMan::PlaySkillVoice (0x19d3de0)

    if (!m_bInitialized)
    {
        return;
    }

    // Stop current voice
    StopSkillVoice();

    std::uint32_t actualVolume = nVolume128 * m_uVoiceVolume / 100;

    // Load and decode audio
    auto mp3Data = LoadSoundFromWZ(sPath);
    if (mp3Data.empty())
    {
        LOG_WARN("Failed to load skill voice: {}", sPath);
        return;
    }

    auto decodedAudio = DecodeMP3(mp3Data);
    if (decodedAudio.samples.empty())
    {
        LOG_WARN("Failed to decode skill voice: {}", sPath);
        return;
    }

    m_pVoiceState = std::make_unique<SoundState>();
    m_pVoiceState->sPath = sPath;
    m_pVoiceState->nVolume = static_cast<std::int32_t>(actualVolume);
    m_pVoiceState->bLooping = bLoop;
    m_pVoiceState->bPlaying = true;
    m_pVoiceState->decodedAudio = std::move(decodedAudio);
    m_pVoiceState->pAudioStream = PlayAudio(m_pVoiceState->decodedAudio,
                                             static_cast<int>(actualVolume),
                                             bLoop);
}

void SoundMan::StopSkillVoice()
{
    // Based on CSoundMan::StopSkillVoice (0xfb83b0)

    if (m_pVoiceState)
    {
        LOG_DEBUG("StopSkillVoice: {}", m_pVoiceState->sPath);

        if (m_pVoiceState->pAudioStream)
        {
            DestroyAudioStream(m_pVoiceState->pAudioStream);
            m_pVoiceState->pAudioStream = nullptr;
        }

        m_pVoiceState->bPlaying = false;
        m_pVoiceState.reset();
    }
}

auto SoundMan::IsSkillVoicePlaying() const -> bool
{
    // Based on CSoundMan::IsSkillVoicePlaying (0xfb8080)
    return m_pVoiceState && m_pVoiceState->bPlaying;
}

void SoundMan::SetSkillVoiceVolume(std::uint32_t nVolume)
{
    // Based on CSoundMan::SetSkillVoiceVolume (0xfb8030)

    std::uint32_t oldVolume = m_uVoiceVolume;
    m_uVoiceVolume = std::min(nVolume, 100u);

    LOG_DEBUG("SetSkillVoiceVolume: {} -> {}", oldVolume, m_uVoiceVolume);

    // Update voice state volume
    if (m_pVoiceState && m_pVoiceState->bPlaying && oldVolume > 0)
    {
        std::int32_t newVolume = m_pVoiceState->nVolume *
                                  static_cast<std::int32_t>(m_uVoiceVolume) /
                                  static_cast<std::int32_t>(oldVolume);
        m_pVoiceState->nVolume = newVolume;
    }
}

// ========== Update ==========

void SoundMan::Update()
{
    // Called each frame to update sound states
    // Handle looping by refilling audio buffers

    // Update BGM looping
    if (m_pBGMState && m_pBGMState->bPlaying && m_pBGMState->bLooping &&
        m_pBGMState->pAudioStream)
    {
        // Check if stream needs more data (buffer is running low)
        int queued = SDL_GetAudioStreamQueued(m_pBGMState->pAudioStream);
        if (queued < static_cast<int>(m_pBGMState->decodedAudio.samples.size() * sizeof(std::int16_t) / 4))
        {
            // Refill with audio data
            const std::uint8_t* pcmData = reinterpret_cast<const std::uint8_t*>(
                m_pBGMState->decodedAudio.samples.data());
            int pcmSize = static_cast<int>(
                m_pBGMState->decodedAudio.samples.size() * sizeof(std::int16_t));
            SDL_PutAudioStreamData(m_pBGMState->pAudioStream, pcmData, pcmSize);
        }
    }

    // Update looping sound effects and ambient sounds
    for (auto& [cookie, state] : m_mpLoopingStates)
    {
        if (state && state->bPlaying && state->bLooping && state->pAudioStream)
        {
            int queued = SDL_GetAudioStreamQueued(state->pAudioStream);
            if (queued < static_cast<int>(state->decodedAudio.samples.size() * sizeof(std::int16_t) / 4))
            {
                const std::uint8_t* pcmData = reinterpret_cast<const std::uint8_t*>(
                    state->decodedAudio.samples.data());
                int pcmSize = static_cast<int>(
                    state->decodedAudio.samples.size() * sizeof(std::int16_t));
                SDL_PutAudioStreamData(state->pAudioStream, pcmData, pcmSize);
            }
        }
    }

    // Update exclusive SE looping
    if (m_pExclSEState && m_pExclSEState->bPlaying && m_pExclSEState->bLooping &&
        m_pExclSEState->pAudioStream)
    {
        int queued = SDL_GetAudioStreamQueued(m_pExclSEState->pAudioStream);
        if (queued < static_cast<int>(m_pExclSEState->decodedAudio.samples.size() * sizeof(std::int16_t) / 4))
        {
            const std::uint8_t* pcmData = reinterpret_cast<const std::uint8_t*>(
                m_pExclSEState->decodedAudio.samples.data());
            int pcmSize = static_cast<int>(
                m_pExclSEState->decodedAudio.samples.size() * sizeof(std::int16_t));
            SDL_PutAudioStreamData(m_pExclSEState->pAudioStream, pcmData, pcmSize);
        }
    }

    // Update skill voice looping
    if (m_pVoiceState && m_pVoiceState->bPlaying && m_pVoiceState->bLooping &&
        m_pVoiceState->pAudioStream)
    {
        int queued = SDL_GetAudioStreamQueued(m_pVoiceState->pAudioStream);
        if (queued < static_cast<int>(m_pVoiceState->decodedAudio.samples.size() * sizeof(std::int16_t) / 4))
        {
            const std::uint8_t* pcmData = reinterpret_cast<const std::uint8_t*>(
                m_pVoiceState->decodedAudio.samples.data());
            int pcmSize = static_cast<int>(
                m_pVoiceState->decodedAudio.samples.size() * sizeof(std::int16_t));
            SDL_PutAudioStreamData(m_pVoiceState->pAudioStream, pcmData, pcmSize);
        }
    }

    // Process fade effects for all active sounds
    UpdateFadeEffects();
}

void SoundMan::UpdateFadeEffects()
{
    auto currentTime = static_cast<std::uint32_t>(SDL_GetTicks());

    // Helper lambda to update a single sound's fade
    auto updateFade = [currentTime](SoundState* state) {
        if (!state || state->fadeType == FadeType::None)
        {
            return;
        }

        auto elapsed = currentTime - state->nFadeStartTime;
        if (elapsed >= state->nFadeDuration)
        {
            // Fade complete
            state->nVolume = state->nFadeEndVolume;
            state->fadeType = FadeType::None;

            // Update audio stream volume
            if (state->pAudioStream)
            {
                float gain = static_cast<float>(state->nVolume) / 128.0f;
                SDL_SetAudioStreamGain(state->pAudioStream, gain);
            }

            return;
        }

        // Calculate interpolated volume
        float progress = static_cast<float>(elapsed) / static_cast<float>(state->nFadeDuration);
        float volumeRange = static_cast<float>(state->nFadeEndVolume - state->nFadeStartVolume);
        state->nVolume = state->nFadeStartVolume + static_cast<std::int32_t>(volumeRange * progress);

        // Update audio stream volume
        if (state->pAudioStream)
        {
            float gain = static_cast<float>(state->nVolume) / 128.0f;
            SDL_SetAudioStreamGain(state->pAudioStream, gain);
        }
    };

    // Update BGM fade
    if (m_pBGMState)
    {
        updateFade(m_pBGMState.get());
        if (m_pBGMState->bStopAfterFade && m_pBGMState->fadeType == FadeType::None)
        {
            // Fade out complete, now stop
            if (m_pBGMState->pAudioStream)
            {
                DestroyAudioStream(m_pBGMState->pAudioStream);
                m_pBGMState->pAudioStream = nullptr;
            }
            m_pBGMState->bPlaying = false;
            m_pBGMState.reset();
            m_sBGMPath.clear();
        }
    }

    // Update exclusive SE fade
    if (m_pExclSEState)
    {
        updateFade(m_pExclSEState.get());
    }

    // Update voice fade
    if (m_pVoiceState)
    {
        updateFade(m_pVoiceState.get());
    }

    // Update looping states fade (ambient and looping SE)
    std::vector<std::uint32_t> toRemove;
    for (auto& [cookie, state] : m_mpLoopingStates)
    {
        if (state)
        {
            updateFade(state.get());
            if (state->bStopAfterFade && state->fadeType == FadeType::None)
            {
                // Fade out complete, mark for removal
                if (state->pAudioStream)
                {
                    DestroyAudioStream(state->pAudioStream);
                    state->pAudioStream = nullptr;
                }
                toRemove.push_back(cookie);
            }
        }
    }

    // Remove stopped sounds
    for (auto cookie : toRemove)
    {
        m_mpLoopingStates.erase(cookie);
        m_mAmbientSound.erase(cookie);
    }
}

// ========== Private Methods ==========

auto SoundMan::LoadSoundFromWZ(const std::string& sPath) -> std::vector<std::uint8_t>
{
    // Load audio data from WZ files
    // Sound files in WZ are typically in "Sound/..." paths
    // Format: WzSoundData contains metadata (length, frequency, size, offset)
    // Actual audio data is raw MP3

    auto& resMan = WzResMan::GetInstance();
    auto prop = resMan.GetProperty(sPath);

    if (!prop)
    {
        LOG_DEBUG("Failed to load sound from WZ: {}", sPath);
        return {};
    }

    // Get sound metadata for logging
    auto soundMeta = prop->GetSound();
    if (soundMeta.size == 0)
    {
        LOG_DEBUG("Sound property has no audio data: {}", sPath);
        return {};
    }

    LOG_DEBUG("Sound metadata: length={}, freq={}, size={}, offset={}",
              soundMeta.length, soundMeta.frequency, soundMeta.size, soundMeta.offset);

    // Load raw audio bytes from WZ file
    auto data = resMan.LoadSoundData(prop);
    if (data.empty())
    {
        LOG_DEBUG("Failed to load sound data: {}", sPath);
        return {};
    }

    LOG_DEBUG("Loaded sound data: {} bytes", data.size());
    return data;
}

auto SoundMan::GetOrCreateCachedSE(const std::string& sPath) -> SECacheItem*
{
    // Check if already cached
    auto it = m_mposSECache.find(sPath);
    if (it != m_mposSECache.end())
    {
        // Update last play time
        auto& item = m_heapSECache[it->second];
        item.nLastPlayTime = static_cast<std::uint32_t>(SDL_GetTicks());
        return &item;
    }

    // Need to load and cache
    auto data = LoadSoundFromWZ(sPath);
    if (data.empty())
    {
        return nullptr;
    }

    // Check if cache is full
    if (m_heapSECache.size() >= MAX_SE_CACHE_SIZE)
    {
        // Flush oldest entries
        FlushSECache(MAX_SE_CACHE_SIZE / 4);
    }

    // Add to cache
    SECacheItem item;
    item.sPath = sPath;
    item.data = std::move(data);
    item.nLastPlayTime = static_cast<std::uint32_t>(SDL_GetTicks());

    m_heapSECache.push_back(std::move(item));
    m_mposSECache[sPath] = m_heapSECache.size() - 1;

    return &m_heapSECache.back();
}

auto SoundMan::DecodeMP3(const std::vector<std::uint8_t>& mp3Data) -> DecodedAudio
{
    DecodedAudio result;

    if (mp3Data.empty())
    {
        return result;
    }

    mp3dec_t dec;
    mp3dec_init(&dec);

    mp3dec_frame_info_t info;
    std::int16_t pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];

    const std::uint8_t* data = mp3Data.data();
    std::size_t remaining = mp3Data.size();

    // Decode all frames
    while (remaining > 0)
    {
        int samples = mp3dec_decode_frame(&dec, data, static_cast<int>(remaining), pcm, &info);

        if (samples > 0)
        {
            // Store channel info from first successful decode
            if (result.channels == 0)
            {
                result.channels = info.channels;
                result.sampleRate = info.hz;
            }

            // Append PCM samples
            std::size_t sampleCount = static_cast<std::size_t>(samples * info.channels);
            result.samples.insert(result.samples.end(), pcm, pcm + sampleCount);
        }

        if (info.frame_bytes == 0)
        {
            // No valid frame found, skip a byte
            data++;
            remaining--;
        }
        else
        {
            data += info.frame_bytes;
            remaining -= static_cast<std::size_t>(info.frame_bytes);
        }
    }

    LOG_DEBUG("Decoded MP3: {} samples, {} channels, {} Hz",
              result.samples.size(), result.channels, result.sampleRate);

    return result;
}

auto SoundMan::PlayAudio(const DecodedAudio& audio, int volume, bool loop) -> SDL_AudioStream*
{
    if (audio.samples.empty() || audio.channels == 0 || audio.sampleRate == 0)
    {
        return nullptr;
    }

    // Create audio spec for the decoded PCM
    SDL_AudioSpec srcSpec;
    srcSpec.format = SDL_AUDIO_S16;
    srcSpec.channels = audio.channels;
    srcSpec.freq = audio.sampleRate;

    // Create audio stream (SDL3 will handle resampling if needed)
    SDL_AudioStream* stream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
        &srcSpec,
        nullptr,  // No callback, we push data
        nullptr
    );

    if (!stream)
    {
        LOG_ERROR("Failed to create audio stream: {}", SDL_GetError());
        return nullptr;
    }

    // Put audio data into stream
    const std::uint8_t* pcmData = reinterpret_cast<const std::uint8_t*>(audio.samples.data());
    int pcmSize = static_cast<int>(audio.samples.size() * sizeof(std::int16_t));

    if (!SDL_PutAudioStreamData(stream, pcmData, pcmSize))
    {
        LOG_ERROR("Failed to put audio data: {}", SDL_GetError());
        SDL_DestroyAudioStream(stream);
        return nullptr;
    }

    // Set volume (SDL3 uses 0.0f - 1.0f)
    float gain = static_cast<float>(volume) / 128.0f;
    SDL_SetAudioStreamGain(stream, gain);

    // Resume playback
    SDL_ResumeAudioStreamDevice(stream);

    // Note: SDL3 doesn't have built-in loop support for streams.
    // Looping is handled in Update() by refilling the buffer when it drains.
    // The 'loop' flag is stored in SoundState and checked there.
    (void)loop;

    return stream;
}

void SoundMan::DestroyAudioStream(SDL_AudioStream* stream)
{
    if (stream)
    {
        SDL_DestroyAudioStream(stream);
    }
}

} // namespace ms

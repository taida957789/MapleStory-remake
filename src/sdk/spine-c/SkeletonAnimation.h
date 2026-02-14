#pragma once

#include <spine/spine.h>

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace ms::spine
{

// Listener typedefs (binary uses COM interfaces; we use std::function)
using StartListener    = std::function<void(int trackIndex)>;
using EndListener      = std::function<void(int trackIndex)>;
using CompleteListener = std::function<void(int trackIndex, int loopCount)>;
using EventListener    = std::function<void(int trackIndex, spEvent* event)>;

// Cached spine metadata for factory creation
struct SpineMetaDatas
{
    spAtlas* pAtlas                           = nullptr;
    spSkeletonData* pSkeletonData             = nullptr;
    spAnimationStateData* pAnimationStateData = nullptr;
    bool bPremultiflyAlpha                    = false; // original spelling
};

// Per-track listener storage (attached to spTrackEntry::rendererObject)
struct TrackEntryListeners
{
    StartListener startListener;
    EndListener endListener;
    CompleteListener completeListener;
    EventListener eventListener;
};

class SkeletonAnimation : public std::enable_shared_from_this<SkeletonAnimation>
{
public:
    // --- Factory methods ---
    static auto CreateWithData(spSkeletonData* pSkeletonData,
                               spAnimationStateData* pAnimationStateData,
                               bool bPremultiflyAlpha) -> std::shared_ptr<SkeletonAnimation>;

    static auto CreateWithFile(const std::string& sPath, bool bBinary) -> std::shared_ptr<SkeletonAnimation>;

    static auto MakeMetaData(const std::string& sPath, bool bBinary) -> SpineMetaDatas;

    ~SkeletonAnimation();

    // --- Update ---
    void update(std::int32_t tCur);
    void postUpdate();

    // --- Animation state ---
    auto setAnimation(int trackIndex, const char* name, bool loop) -> spTrackEntry*;
    auto setAnimation(int trackIndex, const std::string& name, bool loop) -> spTrackEntry*;
    auto addAnimation(int trackIndex, const char* name, bool loop, float delay = 0) -> spTrackEntry*;
    auto addAnimation(int trackIndex, const std::string& name, bool loop, float delay = 0) -> spTrackEntry*;
    auto getCurrent(int trackIndex) -> const spTrackEntry*;
    void clearTracks();
    void clearTrack(int trackIndex);

    void setMix(const char* from, const char* to, float duration);
    void setMix(const std::string& from, const std::string& to, float duration);
    void setDefaultMix(float duration);

    void setCombination(const std::string& name, bool loop);

    // --- Skeleton accessors ---
    auto getSkeleton() const -> const spSkeleton* { return m_pSkeleton; }
    auto getState() const -> const spAnimationState* { return m_pState; }
    auto getBoundingBox() const -> const spSkeletonBounds* { return m_pBoundingBox; }

    void setBoundingBox();
    void updateWorldTransform();
    void setToSetupPose() const;
    void setBonesToSetupPose() const;
    void setSlotsToSetupPose() const;

    auto findBone(const std::string& name) const -> const spBone*;
    auto findSlot(const std::string& name) const -> const spSlot*;
    auto setSkin(const std::string& name) -> bool;
    auto getAttachment(const std::string& slotName, const std::string& attachmentName) const -> const spAttachment*;
    auto setAttachment(const std::string& slotName, const std::string& attachmentName) -> bool;

    // --- Transform ---
    void setFlipX(bool bFlip);
    void setFlipY(bool bFlip);

    // --- Display properties ---
    auto getVisible() const -> bool { return m_bVisible; }
    void setVisible(bool bVisible);
    void setGray(bool bGray);
    auto isUsingPMA() const -> bool { return m_bPremultiflyAlpha; }

    // --- Timing ---
    void setTimeScale(float fTimeScale) { m_fTimeScale = fTimeScale; }
    auto getTimeScale() const -> float { return m_fTimeScale; }

    // --- Global listeners ---
    void setStartListener(StartListener listener)       { m_startListener = std::move(listener); }
    void setEndListener(EndListener listener)            { m_endListener = std::move(listener); }
    void setCompleteListener(CompleteListener listener)  { m_completeListener = std::move(listener); }
    void setEventListener(EventListener listener)        { m_eventListener = std::move(listener); }

    // --- Per-track listeners ---
    void setTrackStartListener(spTrackEntry* entry, StartListener listener);
    void setTrackEndListener(spTrackEntry* entry, EndListener listener);
    void setTrackCompleteListener(spTrackEntry* entry, CompleteListener listener);
    void setTrackEventListener(spTrackEntry* entry, EventListener listener);

    // --- Event callbacks (called from spine-c) ---
    void onAnimationStateEvent(int trackIndex, spEventType type, spEvent* event, int loopCount);
    void onTrackEntryEvent(int trackIndex, spEventType type, spEvent* event, int loopCount);

    // --- Per-slot color overrides ---
    void SetSlotForcedColor(const std::string& sSlotName, std::uint32_t dwColor);
    void ResetSlotForcedColor(const std::string& sSlotName);
    auto AdjustColorByForced(const std::string& sSlotName,
                             std::uint8_t baseA, std::uint8_t baseR,
                             std::uint8_t baseG, std::uint8_t baseB) const -> std::uint32_t;

    // --- Cache management ---
    static void FlushCache(const SpineMetaDatas& data);

private:
    SkeletonAnimation(spSkeletonData* pSkeletonData,
                      spAnimationStateData* pAnimationStateData,
                      bool bPremultiflyAlpha);

    // Non-copyable
    SkeletonAnimation(const SkeletonAnimation&)            = delete;
    SkeletonAnimation& operator=(const SkeletonAnimation&) = delete;

    // --- spine-c owned objects ---
    spSkeleton* m_pSkeleton              = nullptr;
    spAnimationState* m_pState           = nullptr;
    spAnimationState* m_pStateForCombination = nullptr;
    spSkeletonBounds* m_pBoundingBox     = nullptr;

    // --- Display state ---
    bool m_bVisible                      = true;
    bool m_bGray                         = false;
    bool m_bPremultiflyAlpha             = false; // original binary spelling

    // --- Timing ---
    float m_fTimeScale                   = 1.0f;
    bool m_bInterpolatedBetweenKeys      = true;
    std::int32_t m_tLastUpdate           = 0;

    // --- Global listeners ---
    StartListener m_startListener;
    EndListener m_endListener;
    CompleteListener m_completeListener;
    EventListener m_eventListener;

    // --- Per-slot vertex buffers ---
    std::vector<std::vector<float>> m_aVertices;

    // --- Per-slot color overrides ---
    std::map<std::string, std::uint32_t> m_mSlotForcedColor;

    // --- Metadata cache (static) ---
    static std::map<std::string, SpineMetaDatas> s_mCache;
};

} // namespace ms::spine

#include "SkeletonAnimation.h"

#include <spine/extension.h>

#include <cstring>

namespace ms::spine
{

// ---------------------------------------------------------------------------
// Static cache
// ---------------------------------------------------------------------------

std::map<std::string, SpineMetaDatas> SkeletonAnimation::s_mCache;

// ---------------------------------------------------------------------------
// spine-c callbacks (free functions matching spAnimationStateListener signature)
// ---------------------------------------------------------------------------

static void animationCallback(spAnimationState* state, int trackIndex,
                               spEventType type, spEvent* event, int loopCount)
{
    auto* self = static_cast<SkeletonAnimation*>(state->rendererObject);
    self->onAnimationStateEvent(trackIndex, type, event, loopCount);
}

static void trackEntryCallback(spAnimationState* state, int trackIndex,
                                spEventType type, spEvent* event, int loopCount)
{
    auto* self = static_cast<SkeletonAnimation*>(state->rendererObject);
    self->onTrackEntryEvent(trackIndex, type, event, loopCount);
}

// Custom dispose callback that frees per-track TrackEntryListeners before
// the default spine-c _spTrackEntry_dispose runs. This matches the binary's
// spine::disposeTrackEntry pattern.
static void disposeTrackEntry(spTrackEntry* entry)
{
    if (entry->rendererObject)
    {
        delete static_cast<TrackEntryListeners*>(entry->rendererObject);
        entry->rendererObject = nullptr;
    }
    _spTrackEntry_dispose(entry);
}

static auto getListeners(spTrackEntry* entry) -> TrackEntryListeners*
{
    if (!entry->rendererObject)
    {
        entry->rendererObject = new TrackEntryListeners();
        entry->listener       = trackEntryCallback;
    }
    return static_cast<TrackEntryListeners*>(entry->rendererObject);
}

// ---------------------------------------------------------------------------
// Constructor (private)
// ---------------------------------------------------------------------------

SkeletonAnimation::SkeletonAnimation(spSkeletonData* pSkeletonData,
                                     spAnimationStateData* pAnimationStateData,
                                     bool bPremultiflyAlpha)
    : m_bPremultiflyAlpha(bPremultiflyAlpha)
{
    m_pSkeleton = spSkeleton_create(pSkeletonData);
    m_pState    = spAnimationState_create(pAnimationStateData);

    m_pState->rendererObject = this;
    m_pState->listener       = animationCallback;

    // Replace default track entry dispose so TrackEntryListeners are freed
    // automatically when spine-c disposes a track entry (matches binary behavior)
    SUB_CAST(_spAnimationState, m_pState)->disposeTrackEntry = disposeTrackEntry;

    spSkeleton_setToSetupPose(m_pSkeleton);
    spSkeleton_updateWorldTransform(m_pSkeleton);

    // Pre-allocate per-slot vertex buffers (0x960 = 2400 bytes = 600 floats per slot)
    m_aVertices.resize(static_cast<std::size_t>(m_pSkeleton->slotsCount));
    for (auto& buf : m_aVertices)
        buf.resize(600);

    m_tLastUpdate = 0;
}

// ---------------------------------------------------------------------------
// Destructor
// ---------------------------------------------------------------------------

SkeletonAnimation::~SkeletonAnimation()
{
    // spine-c's disposeTrackEntry callback handles TrackEntryListeners cleanup
    if (m_pState)
        spAnimationState_dispose(m_pState);

    if (m_pStateForCombination)
        spAnimationState_dispose(m_pStateForCombination);

    if (m_pBoundingBox)
        spSkeletonBounds_dispose(m_pBoundingBox);

    if (m_pSkeleton)
        spSkeleton_dispose(m_pSkeleton);
}

// ---------------------------------------------------------------------------
// Factory methods
// ---------------------------------------------------------------------------

auto SkeletonAnimation::CreateWithData(spSkeletonData* pSkeletonData,
                                        spAnimationStateData* pAnimationStateData,
                                        bool bPremultiflyAlpha)
    -> std::shared_ptr<SkeletonAnimation>
{
    if (!pSkeletonData || !pAnimationStateData)
        return nullptr;

    // Use the private constructor via a helper since make_shared can't access it
    struct MakeSharedHelper : SkeletonAnimation
    {
        MakeSharedHelper(spSkeletonData* d, spAnimationStateData* a, bool p)
            : SkeletonAnimation(d, a, p) {}
    };

    return std::make_shared<MakeSharedHelper>(pSkeletonData, pAnimationStateData, bPremultiflyAlpha);
}

auto SkeletonAnimation::CreateWithFile(const std::string& sPath, bool bBinary)
    -> std::shared_ptr<SkeletonAnimation>
{
    // Check cache first
    auto it = s_mCache.find(sPath);
    if (it != s_mCache.end())
    {
        auto& data = it->second;
        return CreateWithData(data.pSkeletonData, data.pAnimationStateData, data.bPremultiflyAlpha);
    }

    // Create fresh metadata
    SpineMetaDatas data = MakeMetaData(sPath, bBinary);
    if (!data.pSkeletonData || !data.pAnimationStateData)
        return nullptr;

    s_mCache[sPath] = data;
    return CreateWithData(data.pSkeletonData, data.pAnimationStateData, data.bPremultiflyAlpha);
}

auto SkeletonAnimation::MakeMetaData(const std::string& sPath, bool bBinary) -> SpineMetaDatas
{
    SpineMetaDatas result{};

    // Load atlas
    std::string atlasPath = sPath + ".atlas";
    spAtlas* pAtlas = spAtlas_createFromFile(atlasPath.c_str(), nullptr);
    if (!pAtlas)
        return result;

    // Create JSON reader (reused for both JSON and binary loading)
    spSkeletonJson* pJson = spSkeletonJson_create(pAtlas);
    if (!pJson)
        return result;

    // Read skeleton data
    // Binary: sPath used directly (no extension — matches original binary behavior)
    // JSON:   sPath + ".json"
    spSkeletonData* pSkeletonData = nullptr;
    if (bBinary)
    {
        pSkeletonData = spSkeletonJson_readSkeletonBinaryFile(pJson, sPath.c_str());
    }
    else
    {
        std::string jsonPath = sPath + ".json";
        pSkeletonData = spSkeletonJson_readSkeletonDataFile(pJson, jsonPath.c_str());
    }

    spSkeletonJson_dispose(pJson);

    if (!pSkeletonData)
        return result;

    // Create animation state data
    spAnimationStateData* pStateData = spAnimationStateData_create(pSkeletonData);
    if (!pStateData)
        return result;

    result.pAtlas              = pAtlas;
    result.pSkeletonData       = pSkeletonData;
    result.pAnimationStateData = pStateData;
    result.bPremultiflyAlpha   = false; // TODO: detect PMA from WZ property

    return result;
}

void SkeletonAnimation::FlushCache(const SpineMetaDatas& data)
{
    if (data.pAnimationStateData)
        spAnimationStateData_dispose(data.pAnimationStateData);
    if (data.pSkeletonData)
        spSkeletonData_dispose(data.pSkeletonData);
    if (data.pAtlas)
        spAtlas_dispose(data.pAtlas);
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void SkeletonAnimation::update(std::int32_t tCur)
{
    std::int32_t nElapsed = tCur - m_tLastUpdate;
    if (!m_bInterpolatedBetweenKeys && nElapsed < 33)
        return;

    float fDelta  = static_cast<float>(nElapsed) / 1000.0f;
    float fScaled = m_fTimeScale * fDelta;

    spSkeleton_update(m_pSkeleton, fScaled);
    spAnimationState_update(m_pState, fScaled);

    if (m_pStateForCombination)
        spAnimationState_update(m_pStateForCombination, fScaled);

    spAnimationState_apply(m_pState, m_pSkeleton);

    if (m_pStateForCombination)
        spAnimationState_apply(m_pStateForCombination, m_pSkeleton);

    spSkeleton_updateWorldTransform(m_pSkeleton);

    if (m_pBoundingBox)
        spSkeletonBounds_update(m_pBoundingBox, m_pSkeleton, 1 /*updateAabb*/);

    m_tLastUpdate = tCur;
}

void SkeletonAnimation::postUpdate()
{
    if (!m_pSkeleton)
        return;

    // In the binary, postUpdate syncs skeleton x/y from the center COM layer.
    // Without the COM layer system, callers should set m_pSkeleton->x/y directly.
}

// ---------------------------------------------------------------------------
// Animation state
// ---------------------------------------------------------------------------

auto SkeletonAnimation::setAnimation(int trackIndex, const char* name, bool loop)
    -> spTrackEntry*
{
    spAnimation* anim = spSkeletonData_findAnimation(m_pSkeleton->data, name);
    if (!anim)
        return nullptr;
    return spAnimationState_setAnimation(m_pState, trackIndex, anim, loop);
}

auto SkeletonAnimation::setAnimation(int trackIndex, const std::string& name, bool loop)
    -> spTrackEntry*
{
    return setAnimation(trackIndex, name.c_str(), loop);
}

auto SkeletonAnimation::addAnimation(int trackIndex, const char* name, bool loop, float delay)
    -> spTrackEntry*
{
    spAnimation* anim = spSkeletonData_findAnimation(m_pSkeleton->data, name);
    if (!anim)
        return nullptr;
    return spAnimationState_addAnimation(m_pState, trackIndex, anim, loop, delay);
}

auto SkeletonAnimation::addAnimation(int trackIndex, const std::string& name, bool loop, float delay)
    -> spTrackEntry*
{
    return addAnimation(trackIndex, name.c_str(), loop, delay);
}

auto SkeletonAnimation::getCurrent(int trackIndex) -> const spTrackEntry*
{
    return spAnimationState_getCurrent(m_pState, trackIndex);
}

void SkeletonAnimation::clearTracks()
{
    spAnimationState_clearTracks(m_pState);
}

void SkeletonAnimation::clearTrack(int trackIndex)
{
    spAnimationState_clearTrack(m_pState, trackIndex);
}

void SkeletonAnimation::setMix(const char* from, const char* to, float duration)
{
    spAnimationStateData_setMixByName(m_pState->data, from, to, duration);
}

void SkeletonAnimation::setMix(const std::string& from, const std::string& to, float duration)
{
    setMix(from.c_str(), to.c_str(), duration);
}

void SkeletonAnimation::setDefaultMix(float duration)
{
    m_pState->data->defaultMix = duration;
}

void SkeletonAnimation::setCombination(const std::string& name, bool loop)
{
    if (!m_pStateForCombination)
        m_pStateForCombination = spAnimationState_create(m_pState->data);

    spAnimation* anim = spSkeletonData_findAnimation(m_pSkeleton->data, name.c_str());
    if (anim)
        spAnimationState_setAnimation(m_pStateForCombination, 0, anim, loop);
}

// ---------------------------------------------------------------------------
// Skeleton accessors
// ---------------------------------------------------------------------------

void SkeletonAnimation::setBoundingBox()
{
    if (!m_pBoundingBox)
    {
        m_pBoundingBox = spSkeletonBounds_create();
        spSkeletonBounds_update(m_pBoundingBox, m_pSkeleton, 1 /*updateAabb*/);
    }
}

void SkeletonAnimation::updateWorldTransform()
{
    spSkeleton_updateWorldTransform(m_pSkeleton);
}

void SkeletonAnimation::setToSetupPose() const
{
    spSkeleton_setToSetupPose(m_pSkeleton);
}

void SkeletonAnimation::setBonesToSetupPose() const
{
    spSkeleton_setBonesToSetupPose(m_pSkeleton);
}

void SkeletonAnimation::setSlotsToSetupPose() const
{
    spSkeleton_setSlotsToSetupPose(m_pSkeleton);
}

auto SkeletonAnimation::findBone(const std::string& name) const -> const spBone*
{
    return spSkeleton_findBone(m_pSkeleton, name.c_str());
}

auto SkeletonAnimation::findSlot(const std::string& name) const -> const spSlot*
{
    return spSkeleton_findSlot(m_pSkeleton, name.c_str());
}

auto SkeletonAnimation::setSkin(const std::string& name) -> bool
{
    return spSkeleton_setSkinByName(m_pSkeleton, name.c_str()) != 0;
}

auto SkeletonAnimation::getAttachment(const std::string& slotName,
                                       const std::string& attachmentName) const
    -> const spAttachment*
{
    return spSkeleton_getAttachmentForSlotName(m_pSkeleton, slotName.c_str(), attachmentName.c_str());
}

auto SkeletonAnimation::setAttachment(const std::string& slotName,
                                       const std::string& attachmentName) -> bool
{
    // Binary passes NULL when attachmentName is empty (detach behavior)
    const char* pAttachName = attachmentName.empty() ? nullptr : attachmentName.c_str();
    return spSkeleton_setAttachment(m_pSkeleton, slotName.c_str(), pAttachName) != 0;
}

// ---------------------------------------------------------------------------
// Transform
// ---------------------------------------------------------------------------

void SkeletonAnimation::setFlipX(bool bFlip)
{
    m_pSkeleton->flipX = bFlip ? 1 : 0;
}

void SkeletonAnimation::setFlipY(bool bFlip)
{
    m_pSkeleton->flipY = bFlip ? 1 : 0;
}

// ---------------------------------------------------------------------------
// Display properties
// ---------------------------------------------------------------------------

void SkeletonAnimation::setVisible(bool bVisible)
{
    m_bVisible = bVisible;
    // In binary, also iterates m_aSlotLayer calling put_visible on each COM layer.
    // Without the COM layer system this is a no-op beyond storing the flag.
}

void SkeletonAnimation::setGray(bool bGray)
{
    m_bGray = bGray;
    // In binary, iterates m_aSlotLayer setting grayscale mode on each COM layer.
}

// ---------------------------------------------------------------------------
// Per-track listeners
// ---------------------------------------------------------------------------

void SkeletonAnimation::setTrackStartListener(spTrackEntry* entry, StartListener listener)
{
    getListeners(entry)->startListener = std::move(listener);
}

void SkeletonAnimation::setTrackEndListener(spTrackEntry* entry, EndListener listener)
{
    getListeners(entry)->endListener = std::move(listener);
}

void SkeletonAnimation::setTrackCompleteListener(spTrackEntry* entry, CompleteListener listener)
{
    getListeners(entry)->completeListener = std::move(listener);
}

void SkeletonAnimation::setTrackEventListener(spTrackEntry* entry, EventListener listener)
{
    getListeners(entry)->eventListener = std::move(listener);
}

// ---------------------------------------------------------------------------
// Event dispatch
// ---------------------------------------------------------------------------

void SkeletonAnimation::onAnimationStateEvent(int trackIndex, spEventType type,
                                               spEvent* event, int loopCount)
{
    switch (type)
    {
    case SP_ANIMATION_START:
        if (m_startListener)
            m_startListener(trackIndex);
        break;
    case SP_ANIMATION_END:
        if (m_endListener)
            m_endListener(trackIndex);
        break;
    case SP_ANIMATION_COMPLETE:
        if (m_completeListener)
            m_completeListener(trackIndex, loopCount);
        break;
    case SP_ANIMATION_EVENT:
        if (m_eventListener)
            m_eventListener(trackIndex, event);
        break;
    }
}

void SkeletonAnimation::onTrackEntryEvent(int trackIndex, spEventType type,
                                           spEvent* event, int loopCount)
{
    spTrackEntry* entry = spAnimationState_getCurrent(m_pState, trackIndex);
    if (!entry || !entry->rendererObject)
        return;

    auto* listeners = static_cast<TrackEntryListeners*>(entry->rendererObject);
    switch (type)
    {
    case SP_ANIMATION_START:
        if (listeners->startListener)
            listeners->startListener(trackIndex);
        break;
    case SP_ANIMATION_END:
        if (listeners->endListener)
            listeners->endListener(trackIndex);
        // TrackEntryListeners cleanup is handled by disposeTrackEntry callback
        break;
    case SP_ANIMATION_COMPLETE:
        if (listeners->completeListener)
            listeners->completeListener(trackIndex, loopCount);
        break;
    case SP_ANIMATION_EVENT:
        if (listeners->eventListener)
            listeners->eventListener(trackIndex, event);
        break;
    }
}

// ---------------------------------------------------------------------------
// Per-slot color overrides
// ---------------------------------------------------------------------------

void SkeletonAnimation::SetSlotForcedColor(const std::string& sSlotName, std::uint32_t dwColor)
{
    if (!sSlotName.empty())
        m_mSlotForcedColor[sSlotName] = dwColor;
}

void SkeletonAnimation::ResetSlotForcedColor(const std::string& sSlotName)
{
    if (!sSlotName.empty())
        m_mSlotForcedColor.erase(sSlotName);
}

auto SkeletonAnimation::AdjustColorByForced(const std::string& sSlotName,
                                             std::uint8_t baseA, std::uint8_t baseR,
                                             std::uint8_t baseG, std::uint8_t baseB) const
    -> std::uint32_t
{
    auto it = m_mSlotForcedColor.find(sSlotName);
    if (it == m_mSlotForcedColor.end())
    {
        // No forced color — return base ARGB unchanged
        return (static_cast<std::uint32_t>(baseA) << 24) |
               (static_cast<std::uint32_t>(baseR) << 16) |
               (static_cast<std::uint32_t>(baseG) << 8) |
               static_cast<std::uint32_t>(baseB);
    }

    // Multiply each channel: result = base * forced / 255
    std::uint32_t forced = it->second;
    auto fA = static_cast<std::uint8_t>((baseA * ((forced >> 24) & 0xFF)) / 255);
    auto fR = static_cast<std::uint8_t>((baseR * ((forced >> 16) & 0xFF)) / 255);
    auto fG = static_cast<std::uint8_t>((baseG * ((forced >> 8) & 0xFF)) / 255);
    auto fB = static_cast<std::uint8_t>((baseB * (forced & 0xFF)) / 255);

    return (static_cast<std::uint32_t>(fA) << 24) |
           (static_cast<std::uint32_t>(fR) << 16) |
           (static_cast<std::uint32_t>(fG) << 8) |
           static_cast<std::uint32_t>(fB);
}

} // namespace ms::spine

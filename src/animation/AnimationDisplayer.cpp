#include "AnimationDisplayer.h"

#include "app/Application.h"
#include "graphics/Gr2DVector.h"
#include "graphics/WzGr2D.h"
#include "graphics/WzGr2DCanvas.h"
#include "graphics/WzGr2DLayer.h"
#include "util/Rand32.h"
#include "wz/WzProperty.h"
#include "wz/WzResMan.h"

#include <algorithm>

namespace ms
{

// ========== AnimationDisplayer::PrepareInfo ==========

auto AnimationDisplayer::PrepareInfo::Update(std::int32_t tCur) -> bool
{
    // TODO: look up avatar from CUserPool by dwCharacterIDForFlip
    // const CAvatar* pOwner = nullptr;
    // if (dwCharacterIDForFlip)
    // {
    //     auto* pUser = CUserPool::GetInstance().GetUser(dwCharacterIDForFlip);
    //     if (pUser)
    //         pOwner = &pUser->GetAvatar();
    // }

    bool bResult = true;

    for (auto& pLayer : apLayer)
    {
        if (!pLayer)
            continue;

        const auto nAnimState = pLayer->get_animationState();

        if (nAnimState != 0)
        {
            // Animation still running — sync flip direction
            if (pFlipLayer)
                pLayer->put_flip(pFlipLayer->get_flip());

            // TODO: override flip from avatar move action when CUserPool available
            // if (pOwner)
            //     pLayer->put_flip((pOwner->m_nMoveAction & 1) == 0 ? 1 : 0);

            bResult = false;
        }
        else
        {
            // Animation finished — release the layer
            pLayer.reset();
        }
    }

    // Force removal if reserved time has elapsed
    if (tReservedRemoveTime != 0 && tCur - tReservedRemoveTime > 0)
        AnimationDisplayer::GetInstance().RemovePrepareAnimation(dwCharacterID);

    return bResult;
}

// ========== AnimationDisplayer::OneTimeInfo ==========

auto AnimationDisplayer::OneTimeInfo::Scale(
    [[maybe_unused]] std::int32_t nScale) -> std::int32_t
{
    // TODO: implement — pLayer->raw_VertexShaderConstSet(8, nScale)
    return 0;
}

// ========== AnimationDisplayer::RegisterOneTimeAnimation ==========

auto AnimationDisplayer::RegisterOneTimeAnimation(
    const std::shared_ptr<WzGr2DLayer>& pLayer,
    std::int32_t tDelayBeforeStart,
    const std::shared_ptr<WzGr2DLayer>& pFlipOrigin,
    std::int32_t nDelayRate,
    std::int32_t nMovingType,
    const std::shared_ptr<RelOffsetParam>& pRelOffsetParam,
    const ZXString<wchar_t>& sSoundUOL,
    std::int32_t nComboKillCount,
    std::uint32_t dwOwner
) -> OneTimeInfo&
{
    m_lOneTime.emplace_back();
    auto& info = m_lOneTime.back();

    info.pLayer = pLayer;
    info.bWaiting = 0;
    info.tDelayBeforeStart = tDelayBeforeStart;
    info.pFlipOrigin = pFlipOrigin;
    info.dwOwner = dwOwner;
    info.nDelayRate = nDelayRate;
    info.nMovingType = nMovingType;
    info.nCurrentTick = 0;
    info.nPrevScale = -1;
    info.nBaseScale = (nMovingType >= 3 && nMovingType <= 5) ? 75 : 100;
    info.nComboKillCount = nComboKillCount;
    info.pRelOffsetParam = pRelOffsetParam;
    info.sSoundUOL = sSoundUOL;
    info.nAnimationType = 0;

    // If there's a start delay, make layer transparent and mark waiting
    if (tDelayBeforeStart != 0)
    {
        if (pLayer)
            pLayer->put_color(0x00FFFFFF);
        info.bWaiting = 1;
    }

    // Set render mode for moving effects (vtable offset 0x130 = put_renderMode)
    if (nMovingType != 0)
    {
        // TODO: info.pLayer->put_renderMode(2)
        // WzGr2DLayer does not yet expose put_renderMode
    }

    if (nMovingType == 3)
    {
        // Scale to base (75%) immediately
        info.Scale(info.nBaseScale);
    }
    else if (nMovingType == 2 || nMovingType == 5)
    {
        // Blade moving effect: hide previous effect for this owner, store new one
        auto it = m_mBladeMovingEffect.find(dwOwner);
        if (it != m_mBladeMovingEffect.end() && it->second)
            it->second->put_visible(false);

        m_mBladeMovingEffect[dwOwner] = pLayer;
    }

    return info;
}

// ========== AnimationDisplayer::TrembleCtx ==========

void AnimationDisplayer::TrembleCtx::Update(std::int32_t tCur)
{
    if (m_dTrembleForce <= 0.0)
        return;

    if (tCur - m_tTrembleStart <= 0)
        return;

    // Throttle updates by m_tTrembleTerm interval
    if (m_tTrembleTerm != 0)
    {
        // Util::IsOverTime(m_tTrembleLastUpdate, m_tTrembleTerm, tCur)
        if (tCur - m_tTrembleLastUpdate <= m_tTrembleTerm)
            return;
    }

    m_tTrembleLastUpdate = tCur;

    auto* pCenter = get_gr().GetCenterVec();

    if (tCur - m_tTrembleEnd < 0)
    {
        const auto nRange = static_cast<std::int32_t>(m_dTrembleForce + m_dTrembleForce);
        if (nRange > 0)
        {
            const auto uRange = static_cast<std::uint32_t>(nRange);
            auto& rand = detail::get_rand();
            const auto nForce = static_cast<std::int32_t>(m_dTrembleForce);

            const auto shakeX = static_cast<std::int32_t>(
                static_cast<std::uint32_t>(rand.Random()) % uRange) - nForce;
            const auto shakeY = static_cast<std::int32_t>(
                static_cast<std::uint32_t>(rand.Random()) % uRange) - nForce;

            // Instant RelMove: sets local position to (shake + savedRel),
            // creating an EasingNode that absorbs into base on next evaluation.
            pCenter->RelMove(shakeX + m_ptCenterRel.x,
                             shakeY + m_ptCenterRel.y);
        }

        m_dTrembleForce *= m_dTrembleReduction;
    }

    // End tremble if past end time or force too weak
    if (tCur - m_tTrembleEnd >= 0 || m_dTrembleForce < 1.0)
    {
        // Restore center to pre-tremble relative offset
        pCenter->RelMove(m_ptCenterRel.x, m_ptCenterRel.y);
        m_dTrembleForce = 0.0;
    }

    // TODO: if stage is CField and m_dTrembleForce == 0.0:
    //   CField::SetAbleFloat(pField, 1)
    //   pCenter->WrapClip(nullptr, rcViewRange.left, rcViewRange.top,
    //                      rcViewRange.Width(), rcViewRange.Height(), true)
}

// ========== Effect Methods ==========

void AnimationDisplayer::Effect_General(
    const std::string& sUOL,
    std::int32_t nFlip,
    const std::shared_ptr<Gr2DVector>& pOrigin,
    std::int32_t rx,
    std::int32_t ry,
    const std::shared_ptr<WzGr2DLayer>& pOverlay,
    std::int32_t z,
    std::int32_t nMagLevel)
{
    // Extract position from origin vector
    Point2D origin{};
    if (pOrigin)
        origin = {pOrigin->GetX(), pOrigin->GetY()};

    // Load effect layer (alpha hardcoded to 255)
    auto pLayer = LoadLayer(
        sUOL, nFlip, origin, rx, ry,
        pOverlay, z, 255, nMagLevel,
        nullptr, 0, 0, false);

    if (!pLayer)
        return;

    // Start animation (GA_STOP = play once through and stop)
    pLayer->Animate(Gr2DAnimationType::Stop);

    // Register as a one-time animation so Update() drives playback
    RegisterOneTimeAnimation(
        pLayer, 0, nullptr, 0, 0,
        nullptr, {}, 0, 0);
}

// ========== AnimationDisplayer ==========

void AnimationDisplayer::Update()
{
    const auto tCur = static_cast<std::int32_t>(
        Application::GetInstance().GetUpdateTime());

    UpdateWeaponHeadEffect(tCur);
    m_tremble.Update(tCur);

    // Iterate prepare animation list; remove completed entries
    for (auto it = m_lPrepare.begin(); it != m_lPrepare.end(); )
    {
        auto& pInfo = *it;
        ++it; // advance before potential removal

        if (pInfo && pInfo->Update(tCur))
            RemovePrepareAnimation(pInfo->dwCharacterID);
    }

    NonFieldUpdate(tCur);
    UpdateMoveRandSprayEffect(tCur);
    UpdateUpDownEffect(tCur);
    UpdateDelaySetViewEffect();
}

void AnimationDisplayer::UpdateWeaponHeadEffect(
    [[maybe_unused]] std::int32_t tCur)
{
    // TODO: stub
}

void AnimationDisplayer::NonFieldUpdate(
    [[maybe_unused]] std::int32_t tCur)
{
    // TODO: stub
}

void AnimationDisplayer::UpdateMoveRandSprayEffect(
    [[maybe_unused]] std::int32_t tCur)
{
    // TODO: stub
}

void AnimationDisplayer::UpdateUpDownEffect(
    [[maybe_unused]] std::int32_t tCur)
{
    // TODO: stub
}

void AnimationDisplayer::UpdateDelaySetViewEffect()
{
    // TODO: stub
}

void AnimationDisplayer::RemovePrepareAnimation(
    [[maybe_unused]] std::uint32_t dwCharacterID)
{
    m_lPrepare.remove_if([dwCharacterID](const auto& pInfo) {
        return pInfo && pInfo->dwCharacterID == dwCharacterID;
    });
}

// Overload 1: UOL string wrapper
auto AnimationDisplayer::LoadLayer(
    const std::string& layerUOL,
    std::int32_t flip,
    Point2D origin,
    std::int32_t rx, std::int32_t ry,
    std::shared_ptr<WzGr2DLayer> pOverlay,
    std::int32_t z,
    std::int32_t alpha,
    std::int32_t magLevel,
    LayerCanvasInfo* pCanvasInfo,
    std::int32_t nZoom0, std::int32_t nZoom1,
    bool bPostRender
) -> std::shared_ptr<WzGr2DLayer>
{
    auto& resMan = WzResMan::GetInstance();
    auto prop = resMan.GetProperty(layerUOL);
    if (!prop || !prop->HasChildren())
        return nullptr;

    return LoadLayer(prop, flip, origin, rx, ry,
                     std::move(pOverlay), z, alpha, magLevel,
                     pCanvasInfo, nZoom0, nZoom1, bPostRender);
}

// Overload 2: Property-based (from decompiled inner LoadLayer)
auto AnimationDisplayer::LoadLayer(
    const std::shared_ptr<WzProperty>& prop,
    std::int32_t flip,
    Point2D origin,
    std::int32_t rx, std::int32_t ry,
    std::shared_ptr<WzGr2DLayer> pOverlay,
    std::int32_t z,
    std::int32_t alpha,
    std::int32_t magLevel,
    LayerCanvasInfo* pCanvasInfo,
    std::int32_t nZoom0, std::int32_t nZoom1,
    [[maybe_unused]] bool bPostRender
) -> std::shared_ptr<WzGr2DLayer>
{
    (void)magLevel; // Caller-side concern, not applied to layer

    auto& gr = get_gr();
    auto layer = gr.CreateLayer(0, 0, 0, 0, 0);

    // Set flip
    layer->put_flip(flip);

    // Read "z" property to override z-order
    if (auto zProp = prop->GetChild("z"))
        layer->put_z(zProp->GetInt(z));
    else
        layer->put_z(z);

    // Read "blendMode" property
    if (auto blendProp = prop->GetChild("blendMode"))
        layer->put_blend(blendProp->GetInt(0));

    // Set color with alpha
    layer->put_color((static_cast<std::uint32_t>(alpha) << 24) | 0x00FFFFFF);

    // Position: set origin then apply rx/ry offset via RelMove
    layer->SetPosition(origin.x, origin.y);
    if (rx != 0 || ry != 0)
        layer->get_lt()->RelMove(rx, ry, 0, 0, false, true);

    // Visibility
    layer->put_visible(true);

    // Frame loop: iterate numbered children "0", "1", "2", ...
    int i = 0;
    while (auto frameProp = prop->GetChild(std::to_string(i)))
    {
        auto wzCanvas = frameProp->GetCanvas();
        if (!wzCanvas)
            break;
        ++i;

        LayerCanvasInfoSingle infoSingle;
        LoadCanvas(layer, frameProp, pOverlay, nZoom0, nZoom1,
                   pCanvasInfo ? &infoSingle : nullptr);

        if (pCanvasInfo)
            pCanvasInfo->aInfo.push_back(std::move(infoSingle));
    }

    // Read "a0" property - alpha animation
    if (auto a0Prop = prop->GetChild("a0"))
    {
        auto a0 = a0Prop->GetInt(-1);
        if (alpha == 255 && a0 >= 0)
        {
            a0 = std::clamp(a0, 0, 255);
            layer->get_alpha()->RelMove(a0, 255, 0, 0);
        }
    }

    // Store z in canvas info
    if (pCanvasInfo)
    {
        if (auto zProp = prop->GetChild("z"))
            pCanvasInfo->nZ = zProp->GetInt(0);
    }

    return layer;
}

// InsertLayer UOL string overload (from decompiled CAnimationDisplayer::InsertLayer)
// If pLayer is null, delegates to LoadLayer(UOL).
// If pLayer exists, resolves UOL to property and delegates to property-based InsertLayer.
auto AnimationDisplayer::InsertLayer(
    std::shared_ptr<WzGr2DLayer>& pLayer,
    const std::string& layerUOL,
    std::int32_t flip,
    Point2D origin,
    std::int32_t rx, std::int32_t ry,
    std::shared_ptr<WzGr2DLayer> pOverlay,
    std::int32_t z,
    std::int32_t alpha,
    std::int32_t magLevel
) -> std::shared_ptr<WzGr2DLayer>
{
    if (!pLayer)
    {
        // No existing layer — create via UOL LoadLayer
        pLayer = LoadLayer(layerUOL, flip, origin, rx, ry,
                           std::move(pOverlay), z, alpha, magLevel,
                           nullptr, 0, 0, false);
        return pLayer;
    }

    // Existing layer — resolve UOL to property, then delegate
    auto& resMan = WzResMan::GetInstance();
    auto prop = resMan.GetProperty(layerUOL);
    if (!prop || !prop->HasChildren())
        return nullptr;

    return InsertLayer(pLayer, prop, flip, origin, rx, ry,
                       std::move(pOverlay), z, alpha, magLevel);
}

// InsertLayer property-based overload (from decompiled CAnimationDisplayer::InsertLayer)
// If pLayer is null, creates a new layer via LoadLayer.
// If pLayer exists, appends numbered frame children from prop to the existing layer.
auto AnimationDisplayer::InsertLayer(
    std::shared_ptr<WzGr2DLayer>& pLayer,
    const std::shared_ptr<WzProperty>& prop,
    std::int32_t flip,
    Point2D origin,
    std::int32_t rx, std::int32_t ry,
    std::shared_ptr<WzGr2DLayer> pOverlay,
    std::int32_t z,
    std::int32_t alpha,
    std::int32_t magLevel
) -> std::shared_ptr<WzGr2DLayer>
{
    if (!pLayer)
    {
        // No existing layer — create one via LoadLayer with default zoom/postRender
        pLayer = LoadLayer(prop, flip, origin, rx, ry,
                           std::move(pOverlay), z, alpha, magLevel,
                           nullptr, 0, 0, false);
        return pLayer;
    }

    // Existing layer — append frames from prop's numbered children
    int i = 0;
    while (auto frameProp = prop->GetChild(std::to_string(i)))
    {
        auto wzCanvas = frameProp->GetCanvas();
        if (!wzCanvas)
            break;

        // TODO: original calls IWzCanvas::put_mag(magLevel) when magLevel > 0
        // WzGr2DCanvas does not yet support magnification

        LoadCanvas(pLayer, frameProp, nullptr, 0, 0, nullptr);
        ++i;
    }

    return pLayer;
}

// LoadCanvas helper (from decompiled CAnimationDisplayer::LoadCanvas)
void AnimationDisplayer::LoadCanvas(
    const std::shared_ptr<WzGr2DLayer>& layer,
    const std::shared_ptr<WzProperty>& frameProp,
    [[maybe_unused]] const std::shared_ptr<WzGr2DLayer>& overlay,
    std::int32_t globalZoom0, std::int32_t globalZoom1,
    LayerCanvasInfoSingle* pInfoSingle
)
{
    // Read canvas from frameProp
    auto wzCanvas = frameProp->GetCanvas();
    auto canvas = std::make_shared<WzGr2DCanvas>(wzCanvas);

    // Read origin
    if (auto originProp = frameProp->GetChild("origin"))
    {
        auto vec = originProp->GetVector();
        canvas->SetOrigin({vec.x, vec.y});
    }

    // Read per-frame properties
    int delay = 100;
    if (auto delayProp = frameProp->GetChild("delay"))
        delay = delayProp->GetInt(100);

    int a0 = 255;
    int a1 = 255;
    if (auto a0Prop = frameProp->GetChild("a0"))
        a0 = std::clamp(a0Prop->GetInt(255), 0, 255);
    if (auto a1Prop = frameProp->GetChild("a1"))
        a1 = std::clamp(a1Prop->GetInt(255), 0, 255);

    int z0 = globalZoom0;
    int z1 = globalZoom1;
    if (auto z0Prop = frameProp->GetChild("z0"))
        z0 = z0Prop->GetInt(globalZoom0);
    if (auto z1Prop = frameProp->GetChild("z1"))
        z1 = z1Prop->GetInt(globalZoom1);

    // Insert canvas into layer with per-frame params
    layer->InsertCanvas(canvas, delay,
                        static_cast<std::uint8_t>(a0),
                        static_cast<std::uint8_t>(a1),
                        z0, z1);

    // Fill pInfoSingle if requested
    if (pInfoSingle)
    {
        pInfoSingle->nDelay = delay;
        pInfoSingle->bView = true;

        // Read headCount for multi-head direction info
        if (auto headProp = frameProp->GetChild("headCount"))
        {
            int headCount = headProp->GetInt(0);
            for (int h = 0; h < headCount; ++h)
            {
                std::string frontKey = headCount == 1
                    ? "front"
                    : "front" + std::to_string(h);
                std::string rearKey = headCount == 1
                    ? "rear"
                    : "rear" + std::to_string(h);

                auto frontProp = frameProp->GetChild(frontKey);
                auto rearProp = frameProp->GetChild(rearKey);

                Point2D front{};
                Point2D rear{};
                if (frontProp)
                {
                    auto v = frontProp->GetVector();
                    front = {v.x, v.y};
                }
                if (rearProp)
                {
                    auto v = rearProp->GetVector();
                    rear = {v.x, v.y};
                }
                pInfoSingle->aptDir.emplace_back(front, rear);
            }
        }
    }
}

} // namespace ms

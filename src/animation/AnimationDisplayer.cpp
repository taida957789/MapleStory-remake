#include "AnimationDisplayer.h"

#include "graphics/Gr2DVector.h"
#include "graphics/WzGr2D.h"
#include "graphics/WzGr2DCanvas.h"
#include "graphics/WzGr2DLayer.h"
#include "wz/WzProperty.h"
#include "wz/WzResMan.h"

#include <algorithm>

namespace ms
{

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

// InsertLayer (from decompiled CAnimationDisplayer::InsertLayer)
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

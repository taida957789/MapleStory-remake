#pragma once

#include "util/Point.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace ms
{

class WzGr2DLayer;
class WzProperty;

class AnimationDisplayer
{
public:
    /// Per-frame canvas info (from decompiled LAYERCANVASINFOSINGLE)
    struct LayerCanvasInfoSingle
    {
        std::int32_t nDelay{0};
        std::vector<std::pair<Point2D, Point2D>> aptDir;
        bool bView{true};
    };

    /// Output info for a loaded layer (from decompiled LAYERCANVASINFO)
    struct LayerCanvasInfo
    {
        std::int32_t nZ{0};
        std::vector<LayerCanvasInfoSingle> aInfo;
    };

    /// Overload 1: UOL string path - resolves property, delegates to overload 2
    static auto LoadLayer(
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
    ) -> std::shared_ptr<WzGr2DLayer>;

    /// Overload 2: Property-based - creates layer, reads properties, loops frames
    static auto LoadLayer(
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
        bool bPostRender
    ) -> std::shared_ptr<WzGr2DLayer>;

    /// Overload 1: UOL string path - resolves property, delegates to overload 2
    /// When pLayer is null, delegates to LoadLayer(UOL).
    /// When pLayer exists, resolves UOL to property and delegates.
    static auto InsertLayer(
        std::shared_ptr<WzGr2DLayer>& pLayer,
        const std::string& layerUOL,
        std::int32_t flip,
        Point2D origin,
        std::int32_t rx, std::int32_t ry,
        std::shared_ptr<WzGr2DLayer> pOverlay,
        std::int32_t z,
        std::int32_t alpha,
        std::int32_t magLevel
    ) -> std::shared_ptr<WzGr2DLayer>;

    /// Overload 2: Property-based - inserts frames or creates layer.
    /// When pLayer is null, delegates to LoadLayer.
    /// When pLayer exists, appends numbered frame children from prop.
    static auto InsertLayer(
        std::shared_ptr<WzGr2DLayer>& pLayer,
        const std::shared_ptr<WzProperty>& prop,
        std::int32_t flip,
        Point2D origin,
        std::int32_t rx, std::int32_t ry,
        std::shared_ptr<WzGr2DLayer> pOverlay,
        std::int32_t z,
        std::int32_t alpha,
        std::int32_t magLevel
    ) -> std::shared_ptr<WzGr2DLayer>;

private:
    /// Insert a single canvas frame into a layer (from decompiled LoadCanvas)
    static void LoadCanvas(
        const std::shared_ptr<WzGr2DLayer>& layer,
        const std::shared_ptr<WzProperty>& frameProp,
        const std::shared_ptr<WzGr2DLayer>& overlay,
        std::int32_t globalZoom0, std::int32_t globalZoom1,
        LayerCanvasInfoSingle* pInfoSingle
    );
};

} // namespace ms

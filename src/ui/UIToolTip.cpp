#include "UIToolTip.h"

#include "graphics/WzGr2DCanvas.h"
#include "graphics/WzGr2DLayer.h"

namespace ms
{

// =============================================================================
// Constructor / Destructor
// =============================================================================

UIToolTip::UIToolTip()
{
    // CUIToolTip::CUIToolTip at 0x171da80
    // Original constructor is ~0xA243 bytes; loads all font and canvas
    // resources from UI.wz/UIToolTip.img. Stubbed for now.
    // TODO: Load tooltip frame images, fonts, star canvases, etc.
}

UIToolTip::~UIToolTip()
{
    // CUIToolTip::~CUIToolTip at 0x1711dd0
    ClearToolTip();
}

// =============================================================================
// ClearToolTip
// =============================================================================

void UIToolTip::ClearToolTip()
{
    // CUIToolTip::ClearToolTip at 0x16f41b0
    // Recursively clears this and m_pRightSideToolTip
    auto* pTip = this;

    while (pTip)
    {
        pTip->m_nToolTipType = 0;
        pTip->m_nHeight = 0;
        pTip->m_nWidth = 0;
        pTip->m_pLayer.reset();
        pTip->m_pLayerAdditional.reset();
        pTip->m_nLineNo = 0;
        pTip->m_nOptionLineNo = 0;
        pTip->m_nAddidionalOptionLineNo = 0;
        pTip->m_nSoulOptionNo = 0;
        pTip->m_nOptionLineFontType = 0;
        pTip->m_nSkillOptionLineStartIdx = -1;

        for (std::int32_t i = 0; i < kMaxLines; ++i)
        {
            pTip->m_aLineInfo[static_cast<std::size_t>(i)].Clear();
            pTip->m_aOptionLineInfo[static_cast<std::size_t>(i)].Clear();
            pTip->m_aDiffInfo[static_cast<std::size_t>(i)].m_nType = -1;
        }

        pTip = pTip->m_pRightSideToolTip.get();
    }
}

// =============================================================================
// SetBasicInfo
// =============================================================================

void UIToolTip::SetBasicInfo(
    std::int32_t nToolTipType,
    std::int32_t nWidth, std::int32_t nHeight,
    std::int32_t nLineSeparate)
{
    // CUIToolTip::SetBasicInfo at 0x16f4260
    ClearToolTip();

    auto* pFrame = m_bFarmTooltip ? &m_FarmToolTipFrame : &m_ItemToolTipFrame2;

    m_nToolTipType = nToolTipType;

    // Compute width from NW + NE corner widths
    std::int32_t nCornerW = 0;
    if (pFrame->pNW)
        nCornerW += pFrame->pNW->GetWidth();
    if (pFrame->pNE)
        nCornerW += pFrame->pNE->GetWidth();

    m_nWidth = nCornerW < nWidth ? nWidth : nCornerW;

    // Compute height from NW + SW corner heights
    std::int32_t nCornerH = 0;
    if (pFrame->pNW)
        nCornerH += pFrame->pNW->GetHeight();
    if (pFrame->pSW)
        nCornerH += pFrame->pSW->GetHeight();

    m_nHeight = nCornerH < nHeight ? nHeight : nCornerH;

    m_nLineSeparated = nLineSeparate;
}

// =============================================================================
// SetZ
// =============================================================================

void UIToolTip::SetZ(std::int32_t z)
{
    // CUIToolTip::SetZ at 0x75c0a0
    if (m_pLayer)
        m_pLayer->put_z(z);
}

// =============================================================================
// RelMove
// =============================================================================

void UIToolTip::RelMove(std::int32_t nLeft, std::int32_t nTop)
{
    // CUIToolTip::RelMove at 0x14a0150
    if (m_pLayer)
        m_pLayer->RelMove(nLeft, nTop);
}

// =============================================================================
// GetLayer / GetCanvas
// =============================================================================

auto UIToolTip::GetLayer() const -> std::shared_ptr<WzGr2DLayer>
{
    // CUIToolTip::GetLayer at 0x8f1be0
    return m_pLayer;
}

auto UIToolTip::GetCanvas() const -> std::shared_ptr<WzGr2DCanvas>
{
    // CUIToolTip::GetCanvas at 0x8f1c10
    return m_pEquipCanvas;
}

// =============================================================================
// SetToolTip_String
// =============================================================================

void UIToolTip::SetToolTip_String(
    std::int32_t x, std::int32_t y,
    const std::string& sToolTip,
    std::uint32_t /*uColor*/)
{
    // CUIToolTip::SetToolTip_String at 0x16f8e60
    // TODO: Full implementation requires font system (IWzFont)
    // 1. Measure text with m_pFontGen_White
    // 2. SetBasicInfo with measured width/height
    // 3. MakeLayer to create canvas
    // 4. DrawText centered on canvas
    // 5. SetToolTipCover

    (void)x;
    (void)y;
    (void)sToolTip;
}

// =============================================================================
// SetToolTip_String_MultiLine
// =============================================================================

void UIToolTip::SetToolTip_String_MultiLine(
    std::int32_t x, std::int32_t y,
    const std::string& sToolTip,
    std::int32_t nWidth, std::int32_t bTrimLeft)
{
    // CUIToolTip::SetToolTip_String_MultiLine at 0x172e180
    // TODO: Full implementation requires font system
    // 1. Get font via GetFontByType(10)
    // 2. DrawTextSepartedLine to calculate layout
    // 3. SetBasicInfo
    // 4. MakeLayer
    // 5. DrawTextSepartedLine again to render
    // 6. SetToolTipCover

    (void)x;
    (void)y;
    (void)sToolTip;
    (void)nWidth;
    (void)bTrimLeft;
}

// =============================================================================
// SetToolTip_String2
// =============================================================================

void UIToolTip::SetToolTip_String2(
    std::int32_t x, std::int32_t y,
    const std::string& sTitle, const std::string& sDesc,
    std::int32_t bUpDir, std::int32_t bRightDir,
    std::int32_t nFontType, std::int32_t nMaxWidth,
    std::int32_t bHasIcon, std::int32_t bItemToolTip,
    std::uint32_t uColor)
{
    // CUIToolTip::SetToolTip_String2 at 0x172e300
    // TODO: Full implementation requires font system

    (void)x;
    (void)y;
    (void)sTitle;
    (void)sDesc;
    (void)bUpDir;
    (void)bRightDir;
    (void)nFontType;
    (void)nMaxWidth;
    (void)bHasIcon;
    (void)bItemToolTip;
    (void)uColor;
}

// =============================================================================
// SetToolTipCover
// =============================================================================

void UIToolTip::SetToolTipCover()
{
    // CUIToolTip::SetToolTipCover at 0x16f3900
    // TODO: Draws the cover canvas over the tooltip layer
}

// =============================================================================
// AddInfo
// =============================================================================

void UIToolTip::AddInfo(
    std::int32_t nFontType, const std::string& sText,
    std::int32_t nWidth, std::int32_t bMulti)
{
    // CUIToolTip::AddInfo at 0x1740d30
    if (m_nLineNo >= kMaxLines)
        return;

    auto& line = m_aLineInfo[static_cast<std::size_t>(m_nLineNo)];
    line.m_sContext = sText;
    line.m_nWidth = nWidth;
    line.m_bMulti = bMulti;

    // TODO: Measure text height using font from nFontType
    line.m_nHeight = 14; // placeholder
    (void)nFontType;

    ++m_nLineNo;
}

// =============================================================================
// AddDiffInfo
// =============================================================================

void UIToolTip::AddDiffInfo(
    std::int32_t nLineNo, const std::string& sText,
    std::int32_t nType)
{
    // CUIToolTip::AddDiffInfo at 0x16f6cb0
    if (nLineNo < 0 || nLineNo >= kMaxLines)
        return;

    auto& diff = m_aDiffInfo[static_cast<std::size_t>(nLineNo)];
    diff.m_sContext = sText;
    diff.m_nType = nType;
}

// =============================================================================
// DrawText helpers (stubs)
// =============================================================================

void UIToolTip::DrawTextCenter(
    std::int32_t nY, const std::string& sText,
    std::int32_t nFontType)
{
    // CUIToolTip::DrawTextCenter at 0x16f6cb0
    // TODO: Requires canvas + font rendering
    (void)nY;
    (void)sText;
    (void)nFontType;
}

void UIToolTip::DrawTextLeft(
    std::int32_t nY, const std::string& sText,
    std::int32_t nFontType, std::int32_t nX)
{
    // CUIToolTip::DrawTextLeft at 0x16f6f30
    // TODO: Requires canvas + font rendering
    (void)nY;
    (void)sText;
    (void)nFontType;
    (void)nX;
}

void UIToolTip::DrawTextRight(
    std::int32_t nY, const std::string& sText,
    std::int32_t nFontType, std::int32_t nX)
{
    // CUIToolTip::DrawTextRight at 0x16f7110
    // TODO: Requires canvas + font rendering
    (void)nY;
    (void)sText;
    (void)nFontType;
    (void)nX;
}

auto UIToolTip::DrawInfo(
    std::int32_t nY, std::int32_t bDraw) -> std::int32_t
{
    // CUIToolTip::DrawInfo at 0x1741660
    // TODO: Iterate m_aLineInfo and render each line
    (void)nY;
    (void)bDraw;
    return nY;
}

auto UIToolTip::DrawEquipInfo(
    std::int32_t nY, std::int32_t bDraw) -> std::int32_t
{
    // CUIToolTip::DrawEquipInfo at 0x1741c90
    // TODO: Draw equipment stat lines
    (void)nY;
    (void)bDraw;
    return nY;
}

auto UIToolTip::DrawOptionInfo(
    std::int32_t nY,
    std::shared_ptr<WzGr2DCanvas> pCanvas,
    std::int32_t nType,
    std::int32_t bDraw) -> std::int32_t
{
    // CUIToolTip::DrawOptionInfo at 0x17423c0
    // TODO: Draw option lines
    (void)nY;
    (void)pCanvas;
    (void)nType;
    (void)bDraw;
    return nY;
}

// =============================================================================
// InitCanvas
// =============================================================================

void UIToolTip::InitCanvas(
    std::shared_ptr<WzGr2DCanvas> pCanvas,
    std::int32_t bClear, std::uint32_t uColor)
{
    // CUIToolTip::InitCanvas at 0x16f1320
    // TODO: Initialize canvas with background color
    (void)pCanvas;
    (void)bClear;
    (void)uColor;
}

// =============================================================================
// MakeLayer
// =============================================================================

auto UIToolTip::MakeLayer(
    std::int32_t x, std::int32_t y,
    std::int32_t bHasIcon, std::int32_t bUpDir,
    std::int32_t bRightDir,
    std::uint32_t uColor,
    std::int32_t bItemToolTip,
    std::int32_t bNoTrimX) -> std::shared_ptr<WzGr2DCanvas>
{
    // CUIToolTip::MakeLayer at 0x16f1fb0
    // TODO: Create layer + canvas, position tooltip, draw frame
    (void)x;
    (void)y;
    (void)bHasIcon;
    (void)bUpDir;
    (void)bRightDir;
    (void)uColor;
    (void)bItemToolTip;
    (void)bNoTrimX;
    return nullptr;
}

// =============================================================================
// MakeLayerItem
// =============================================================================

auto UIToolTip::MakeLayerItem(
    std::int32_t x, std::int32_t y,
    std::int32_t bHasIcon,
    bool bFarmTooltip) -> std::shared_ptr<WzGr2DCanvas>
{
    // CUIToolTip::MakeLayerItem at 0x16f43a0
    // TODO: Create item-specific layer
    (void)x;
    (void)y;
    (void)bHasIcon;
    (void)bFarmTooltip;
    return nullptr;
}

} // namespace ms

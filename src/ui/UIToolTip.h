#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace ms
{

class WzGr2DCanvas;
class WzGr2DLayer;
class WzProperty;

/**
 * @brief Tooltip rendering class
 *
 * Based on CUIToolTip from the original MapleStory client (v1029).
 *
 * Original: __cppobj CUIToolTip (2672 bytes)
 *
 * Handles all tooltip rendering for items (equip, bundle, pet),
 * skills, NPCs, world map, party quests, and more.
 * Uses a line-based layout system with CLineInfo/DiffInfo arrays.
 *
 * The constructor (0x171da80) is ~0xA243 bytes and loads all font
 * and canvas resources from UI.wz/UIToolTip.img.
 */
class UIToolTip
{
public:
    /// Maximum number of lines in tooltip
    static constexpr std::int32_t kMaxLines = 32;

    // =========================================================================
    // Inner types
    // =========================================================================

    /// Line information for tooltip layout (24 bytes)
    struct CLineInfo
    {
        std::string m_sContext;
        std::int32_t m_nHeight{};
        std::int32_t m_nWidth{};
        std::int32_t m_bMulti{};

        void Clear()
        {
            m_nHeight = 0;
            m_nWidth = 0;
            m_bMulti = 0;
        }
    };

    /// Diff information for stat comparison (8 bytes)
    struct DiffInfo
    {
        std::int32_t m_nType{-1};
        std::string m_sContext;
    };

    /// Fixed-size frame images for simple tooltip (20 bytes)
    struct ITEMTOOLTIPFRAME
    {
        std::shared_ptr<WzGr2DCanvas> pTop;
        std::shared_ptr<WzGr2DCanvas> pBottom;
        std::shared_ptr<WzGr2DCanvas> pLine;
        std::shared_ptr<WzGr2DCanvas> pDotLine;
        std::shared_ptr<WzGr2DCanvas> pCover;
    };

    /// 9-patch variable-size frame (44 bytes)
    struct VARIABLE_FRAME
    {
        std::shared_ptr<WzGr2DCanvas> pNW;
        std::shared_ptr<WzGr2DCanvas> pNE;
        std::shared_ptr<WzGr2DCanvas> pSW;
        std::shared_ptr<WzGr2DCanvas> pSE;
        std::shared_ptr<WzGr2DCanvas> pN;
        std::shared_ptr<WzGr2DCanvas> pS;
        std::shared_ptr<WzGr2DCanvas> pW;
        std::shared_ptr<WzGr2DCanvas> pE;
        std::shared_ptr<WzGr2DCanvas> pC;
        std::shared_ptr<WzGr2DCanvas> pDotLine;
        std::shared_ptr<WzGr2DCanvas> pCover;
    };

    /// Item tooltip icon images (36 bytes)
    struct ITEMTOOLTIPICON
    {
        std::shared_ptr<WzGr2DCanvas> pBase;
        std::shared_ptr<WzGr2DCanvas> pShade;
        std::shared_ptr<WzGr2DCanvas> pCover;
        std::vector<std::shared_ptr<WzGr2DCanvas>> apGradeLine;
        std::shared_ptr<WzGr2DCanvas> pOld;
        std::shared_ptr<WzGr2DCanvas> pNew;
    };

    /// Look-ahead icon images (16 bytes)
    struct ITEMTOOLTIPLOOKAHEAD
    {
        std::shared_ptr<WzGr2DCanvas> pPDDIcon;
        std::shared_ptr<WzGr2DCanvas> pMDDIcon;
        std::shared_ptr<WzGr2DCanvas> pBDRIcon;
        std::shared_ptr<WzGr2DCanvas> pIMPRIcon;
    };

    /// Growth helper images (32 bytes)
    struct GROWTHHELPRIMAGE
    {
        std::shared_ptr<WzGr2DCanvas> pTextKind;
        std::shared_ptr<WzGr2DCanvas> pTextLevel;
        std::shared_ptr<WzGr2DCanvas> pTextStarForce;
        std::shared_ptr<WzGr2DCanvas> pTextExplain;
        std::shared_ptr<WzGr2DCanvas> pItemListTop;
        std::shared_ptr<WzGr2DCanvas> pItemListMid;
        std::shared_ptr<WzGr2DCanvas> pItemListBottom;
        std::shared_ptr<WzGr2DCanvas> pDotLine;
    };

    /// Parameters for ShowItemToolTip (40 bytes)
    struct ItemToolTipParam
    {
        std::string sDonator;
        std::string sReplacedDesc;
        void* pComm{};
        std::int32_t nOriginalPrice{};
        std::int32_t bShowPetLife{};
        std::int32_t bShowPetSkill{};
        std::int32_t bUnrelease{};
        std::int32_t nBodyPartForRightClick{};
        std::int32_t bOnlyNameAndDesc{};
        std::string sAddedNameStr;
    };

    /// Font type indices for GetFontByType
    enum FontType : std::int32_t
    {
        FONT_HL_WHITE = 0,
        FONT_HL_GOLD,
        FONT_HL_ORANGE,
        FONT_HL_GRAY,
        FONT_HL_GREEN,
        FONT_HL_BLUE,
        FONT_HL_VIOLET,
        FONT_HL_GREEN2,
        FONT_HL_EXCELLENT,
        FONT_HL_SPECIAL,
        FONT_GEN_WHITE,
        FONT_GEN_GRAY,
        FONT_GEN_GRAY2,
        FONT_GEN_RED,
        FONT_GEN_ORANGE,
        FONT_GEN_GOLD,
        FONT_GEN_PURPLE,
        FONT_GEN_GREEN,
        FONT_GEN_YELLOW,
        FONT_GEN_BLUE,
        FONT_GEN_UNKNOWN,
        FONT_SMALL_WHITE,
        FONT_SMALL_GRAY,
        FONT_SMALL_GREEN,
        FONT_SMALL_YELLOW,
        FONT_COUNT,
    };

    // =========================================================================
    // Lifecycle
    // =========================================================================

    UIToolTip();
    virtual ~UIToolTip();

    // =========================================================================
    // Core tooltip API
    // =========================================================================

    /// Clear all tooltip data and release layers (0x16f41b0)
    void ClearToolTip();

    /// Initialize basic tooltip dimensions (0x16f4260)
    void SetBasicInfo(std::int32_t nToolTipType,
                      std::int32_t nWidth, std::int32_t nHeight,
                      std::int32_t nLineSeparate);

    /// Set tooltip Z order (0x75c0a0)
    void SetZ(std::int32_t z);

    /// Set parent Z order (0x19ef4a0)
    void SetParentZ(std::int32_t z) { m_nParentZ = z; }

    /// Relative move of tooltip layer (0x14a0150)
    void RelMove(std::int32_t nLeft, std::int32_t nTop);

    /// Get tooltip type (0x5ce100)
    [[nodiscard]] auto GetToolTipType() const noexcept -> std::int32_t
    {
        return m_nToolTipType;
    }

    /// Check if tooltip is visible (width != 0) (0x64acf0)
    [[nodiscard]] auto IsToolTipVisible() const noexcept -> bool
    {
        return m_nWidth != 0;
    }

    /// Get the rendering layer (0x8f1be0)
    [[nodiscard]] auto GetLayer() const -> std::shared_ptr<WzGr2DLayer>;

    /// Get the tooltip canvas (0x8f1c10)
    [[nodiscard]] auto GetCanvas() const -> std::shared_ptr<WzGr2DCanvas>;

    /// Set farm tooltip mode (0x12425c0)
    void SetFarmTooltip(bool bFarm) { m_bFarmTooltip = bFarm; }

    /// Ignore wedding info (0x1696ef0)
    void IgnoreWeddingInfo() { m_bIngoreWeddingInfo = 1; }

    // =========================================================================
    // String tooltip methods
    // =========================================================================

    /// Simple single-line text tooltip (0x16f8e60)
    void SetToolTip_String(std::int32_t x, std::int32_t y,
                           const std::string& sToolTip,
                           std::uint32_t uColor = 0);

    /// Multi-line text tooltip (0x172e180)
    void SetToolTip_String_MultiLine(std::int32_t x, std::int32_t y,
                                     const std::string& sToolTip,
                                     std::int32_t nWidth = 0,
                                     std::int32_t bTrimLeft = 0);

    /// Two-part text tooltip (0x172e300)
    void SetToolTip_String2(std::int32_t x, std::int32_t y,
                            const std::string& sTitle,
                            const std::string& sDesc,
                            std::int32_t bUpDir = 0,
                            std::int32_t bRightDir = 0,
                            std::int32_t nFontType = 0,
                            std::int32_t nMaxWidth = 0,
                            std::int32_t bHasIcon = 0,
                            std::int32_t bItemToolTip = 0,
                            std::uint32_t uColor = 0);

    /// Set cover layer on top of tooltip (0x16f3900)
    void SetToolTipCover();

    // =========================================================================
    // Drawing helpers (internal, used by SetToolTip_* methods)
    // =========================================================================

    /// Add a line of info to the tooltip (0x1740d30)
    void AddInfo(std::int32_t nFontType, const std::string& sText,
                 std::int32_t nWidth, std::int32_t bMulti);

    /// Add diff info (0x16f6cb0)
    void AddDiffInfo(std::int32_t nLineNo, const std::string& sText,
                     std::int32_t nType);

    /// Draw text centered (0x16f6cb0)
    void DrawTextCenter(std::int32_t nY, const std::string& sText,
                        std::int32_t nFontType);

    /// Draw text left-aligned (0x16f6f30)
    void DrawTextLeft(std::int32_t nY, const std::string& sText,
                      std::int32_t nFontType, std::int32_t nX = 0);

    /// Draw text right-aligned (0x16f7110)
    void DrawTextRight(std::int32_t nY, const std::string& sText,
                       std::int32_t nFontType, std::int32_t nX = 0);

    /// Draw line info (0x1741660)
    auto DrawInfo(std::int32_t nY, std::int32_t bDraw) -> std::int32_t;

    /// Draw equip info (0x1741c90)
    auto DrawEquipInfo(std::int32_t nY, std::int32_t bDraw) -> std::int32_t;

    /// Draw option info (0x17423c0)
    auto DrawOptionInfo(std::int32_t nY,
                        std::shared_ptr<WzGr2DCanvas> pCanvas,
                        std::int32_t nType,
                        std::int32_t bDraw) -> std::int32_t;

    // =========================================================================
    // Rendering
    // =========================================================================

    /// Initialize a canvas for drawing (0x16f1320)
    void InitCanvas(std::shared_ptr<WzGr2DCanvas> pCanvas,
                    std::int32_t bClear, std::uint32_t uColor);

    /// Create the tooltip layer (0x16f1fb0)
    auto MakeLayer(std::int32_t x, std::int32_t y,
                   std::int32_t bHasIcon, std::int32_t bUpDir,
                   std::int32_t bRightDir,
                   std::uint32_t uColor,
                   std::int32_t bItemToolTip,
                   std::int32_t bNoTrimX) -> std::shared_ptr<WzGr2DCanvas>;

    /// Create item tooltip layer (0x16f43a0)
    auto MakeLayerItem(std::int32_t x, std::int32_t y,
                       std::int32_t bHasIcon,
                       bool bFarmTooltip) -> std::shared_ptr<WzGr2DCanvas>;

private:
    // =========================================================================
    // Core state
    // =========================================================================
    std::int32_t m_nToolTipType{};
    std::int32_t m_nHeight{};
    std::int32_t m_nWidth{};
    std::shared_ptr<WzGr2DLayer> m_pLayer;
    std::shared_ptr<WzGr2DLayer> m_pLayerAdditional;
    std::shared_ptr<WzGr2DCanvas> m_pEquipCanvas;
    std::int32_t m_nLastX{};
    std::int32_t m_nLastY{};
    std::int32_t m_nLastSkillID{};
    bool m_bFarmTooltip{};

    // =========================================================================
    // Line layout
    // =========================================================================
    std::int32_t m_nLineNo{};
    std::array<CLineInfo, kMaxLines> m_aLineInfo;
    std::array<DiffInfo, kMaxLines> m_aDiffInfo;
    std::int32_t m_nLineSeparated{};
    std::int32_t m_nVariableStatNo{};

    // =========================================================================
    // Option lines
    // =========================================================================
    std::int32_t m_nOptionLineNo{};
    std::int32_t m_nAddidionalOptionLineNo{};
    std::int32_t m_nSoulOptionNo{};
    std::array<CLineInfo, kMaxLines> m_aOptionLineInfo;
    std::int32_t m_nOptionLineFontType{};
    std::int32_t m_nSkillOptionLineStartIdx{-1};

    // =========================================================================
    // Fonts (IWzFont → placeholder indices for now)
    // TODO: Replace with actual font objects when IWzFont is implemented
    // =========================================================================

    // Highlight fonts
    // m_pFontHL_White, m_pFontHL_Gold, m_pFontHL_Orange, etc.
    // General fonts
    // m_pFontGen_White, m_pFontGen_Gray, etc.
    // Small fonts
    // m_pFontSmall_White, etc.
    // Special fonts
    // m_pFont_NoNum_ExBold_16_White, etc.
    // Heading fonts
    // m_pFontH_White, m_pFontHGen_Gold
    // Multi-byte fonts
    // m_pMBFonts[2]
    // Item title/body fonts
    // m_pFontItemTitle*, m_pFontItemBody*

    // =========================================================================
    // WZ canvas/property resources
    // =========================================================================

    /// Equip requirement canvases [6][3]
    std::shared_ptr<WzGr2DCanvas> m_pCanvasEquip_ReqItem[6][3];
    std::shared_ptr<WzGr2DCanvas> m_pCanvasEquip_ReqItem_Old[6][3];

    /// Number properties (can/cannot/disable/lookahead/yellow)
    std::shared_ptr<WzProperty> m_pNumberCan;
    std::shared_ptr<WzProperty> m_pNumberCannot;
    std::shared_ptr<WzProperty> m_pNumberCanOld;
    std::shared_ptr<WzProperty> m_pNumberCannotOld;
    std::shared_ptr<WzProperty> m_pNumberDisable;
    std::shared_ptr<WzProperty> m_pNumberLookAhead;
    std::shared_ptr<WzProperty> m_pNumberYellow;

    /// Growth item canvases [4][2]
    std::shared_ptr<WzGr2DCanvas> m_pCanvasEquip_GrowthItem[4][2];
    std::shared_ptr<WzProperty> m_pNumberGrowthEnable;
    std::shared_ptr<WzProperty> m_pNumberGrowthDisable;

    /// Durability canvases [2][2]
    std::shared_ptr<WzGr2DCanvas> m_pCanvasEquip_Durability[2][2];

    // =========================================================================
    // Misc state
    // =========================================================================
    std::int32_t m_bIngoreWeddingInfo{};
    std::shared_ptr<UIToolTip> m_pRightSideToolTip;

    // =========================================================================
    // Star canvases
    // =========================================================================
    std::shared_ptr<WzGr2DCanvas> m_pEquipStar;
    std::shared_ptr<WzGr2DCanvas> m_pEmptyStar;
    std::shared_ptr<WzGr2DCanvas> m_pAmazingHyperUpgradeStar;

    // =========================================================================
    // AD (additional) number canvases
    // =========================================================================
    std::vector<std::shared_ptr<WzGr2DCanvas>> m_apADNumInc;
    std::vector<std::shared_ptr<WzGr2DCanvas>> m_apADNumDec;
    std::shared_ptr<WzGr2DCanvas> m_pADNumPlus;
    std::shared_ptr<WzGr2DCanvas> m_pADNumMinus;
    std::shared_ptr<WzGr2DCanvas> m_pADNumStay;
    std::shared_ptr<WzGr2DCanvas> m_pADNumDesc;

    // =========================================================================
    // Job requirement canvases
    // =========================================================================
    std::shared_ptr<WzGr2DCanvas> m_pEquipJobReqNorm;
    std::shared_ptr<WzGr2DCanvas> m_pEquipJobReqExpand;
    std::vector<std::shared_ptr<WzGr2DCanvas>> m_apEquipJobReq[2];

    // =========================================================================
    // Growth helper canvases
    // =========================================================================
    std::shared_ptr<WzGr2DCanvas> m_pEquipGrowthBack;
    std::shared_ptr<WzGr2DCanvas> m_pEquipGrowthExpGauge;
    std::shared_ptr<WzGr2DCanvas> m_pEquipGrowthLevPart;
    std::shared_ptr<WzGr2DCanvas> m_pEquipGrowthLevPart2;

    // =========================================================================
    // Frame/icon resources
    // =========================================================================
    ITEMTOOLTIPFRAME m_ItemToolTipFrame;
    VARIABLE_FRAME m_ItemToolTipFrame2;
    VARIABLE_FRAME m_FarmToolTipFrame;
    ITEMTOOLTIPICON m_ItemToolTipIcon;
    ITEMTOOLTIPLOOKAHEAD m_ItemToolTipLookAheadIcon;
    GROWTHHELPRIMAGE m_GrowthHelperImage;
    std::shared_ptr<WzGr2DCanvas> m_pLockSkillIcon;

    // =========================================================================
    // Z-order
    // =========================================================================
    std::int32_t m_nParentZ{};
};

} // namespace ms

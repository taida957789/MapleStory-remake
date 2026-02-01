#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ms
{

class UIElement;
class UIButton;
class WzGr2DLayer;

/**
 * @brief UI 自動化構建和管理系統
 *
 * 基於 IDA Pro 分析 CLayoutMan (MapleStory v1029)
 * 負責從 WZ 資源自動創建 UI 元素並提供統一管理接口
 */
class LayoutMan
{
public:
    LayoutMan();
    ~LayoutMan();

    // Non-copyable, movable
    LayoutMan(const LayoutMan&) = delete;
    auto operator=(const LayoutMan&) -> LayoutMan& = delete;
    LayoutMan(LayoutMan&&) noexcept = default;
    auto operator=(LayoutMan&&) noexcept -> LayoutMan& = default;

    /**
     * @brief 初始化 LayoutMan
     * @param pParent 父 UI 元素
     * @param nOffsetX 全局 X 偏移
     * @param nOffsetY 全局 Y 偏移
     *
     * IDA: CLayoutMan::Init
     */
    void Init(UIElement* pParent, int nOffsetX = 0, int nOffsetY = 0);

    /**
     * @brief 從 WZ 資源自動構建 UI
     * @param sRootUOL WZ 資源根路徑
     * @param nIdBase 控件 ID 起始值
     * @param nOffsetX X 偏移
     * @param nOffsetY Y 偏移
     * @param bSetTooltip 是否設置 tooltip
     * @param bSameIDCtrl 允許相同 ID 控件
     *
     * IDA: CLayoutMan::AutoBuild (0xb36170)
     */
    void AutoBuild(
        const std::wstring& sRootUOL,
        int nIdBase,
        int nOffsetX,
        int nOffsetY,
        bool bSetTooltip,
        bool bSameIDCtrl
    );

    // ========== 按鈕管理 ==========

    /**
     * @brief 按名稱查找按鈕
     * @param sName 按鈕名稱
     * @return 按鈕指針，未找到返回 nullptr
     *
     * IDA: CLayoutMan::ABGetButton (0xb300f0)
     */
    [[nodiscard]] auto ABGetButton(const std::wstring& sName) -> std::shared_ptr<UIButton>;

    /**
     * @brief 設置所有按鈕的顯示狀態
     * @param bShow 是否顯示
     *
     * IDA: CLayoutMan::ABSetButtonShowAll (0xb2e680)
     */
    void ABSetButtonShowAll(bool bShow);

    /**
     * @brief 設置所有按鈕的啟用狀態
     * @param bEnable 是否啟用
     *
     * IDA: CLayoutMan::ABSetButtonEnableAll (0xb2e770)
     */
    void ABSetButtonEnableAll(bool bEnable);

    /**
     * @brief 設置指定按鈕的啟用狀態
     * @param sName 按鈕名稱
     * @param bEnable 是否啟用
     *
     * IDA: CLayoutMan::ABSetButtonEnable (0xb392f0)
     */
    void ABSetButtonEnable(const std::wstring& sName, bool bEnable);

    // ========== 圖層管理 ==========

    /**
     * @brief 註冊圖層到映射表
     * @param pLayer 圖層指針
     * @param sName 圖層名稱
     *
     * IDA: CLayoutMan::RegisterLayer (0xb35d40)
     */
    void RegisterLayer(std::shared_ptr<WzGr2DLayer> pLayer, const std::wstring& sName);

    /**
     * @brief 按名稱查找圖層
     * @param sName 圖層名稱
     * @return 圖層指針，未找到返回 nullptr
     *
     * IDA: CLayoutMan::ABGetLayer (0xb35f90)
     */
    [[nodiscard]] auto ABGetLayer(const std::wstring& sName) -> std::shared_ptr<WzGr2DLayer>;

    /**
     * @brief 設置指定圖層的可見性
     * @param sName 圖層名稱
     * @param bVisible 是否可見
     *
     * IDA: CLayoutMan::ABSetLayerVisible (0xb38ba0)
     */
    void ABSetLayerVisible(const std::wstring& sName, bool bVisible);

    /**
     * @brief 設置所有圖層的可見性
     * @param bVisible 是否可見
     *
     * IDA: CLayoutMan::ABSetLayerVisibleAll (0xb2ce20)
     */
    void ABSetLayerVisibleAll(bool bVisible);

private:
    /**
     * @brief 添加按鈕到管理系統
     * @param sButtonUOL 按鈕 WZ 資源完整路徑
     * @param nID 控件 ID
     * @param nOffsetX X 偏移
     * @param nOffsetY Y 偏移
     * @param bToggle 是否為 toggle 按鈕
     * @param bSkipIDCheck 跳過 ID 檢查
     * @return 創建的按鈕指針
     *
     * IDA: CLayoutMan::AddButton (0xb30310)
     */
    auto AddButton(
        const std::wstring& sButtonUOL,
        unsigned int nID,
        int nOffsetX,
        int nOffsetY,
        bool bToggle = false,
        bool bSkipIDCheck = false
    ) -> std::shared_ptr<UIButton>;

    UIElement* m_pParent{nullptr};  // 父 UI 元素

    // 按鈕映射表（按名稱索引）
    std::map<std::wstring, std::shared_ptr<UIButton>> m_mButtons;

    // 圖層映射表（按名稱索引）
    std::map<std::wstring, std::shared_ptr<WzGr2DLayer>> m_mLayers;

    // 所有控件陣列（按創建順序）
    std::vector<std::shared_ptr<UIElement>> m_aCtrl;

    // 所有圖層陣列
    std::vector<std::shared_ptr<WzGr2DLayer>> m_aLayer;

    // 全局位置偏移
    int m_nOffsetX{0};
    int m_nOffsetY{0};
};

} // namespace ms

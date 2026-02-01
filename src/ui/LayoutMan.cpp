#include "LayoutMan.h"
#include "UIButton.h"
#include "UIElement.h"
#include "graphics/WzGr2DLayer.h"
#include "util/Logger.h"
#include "wz/WzCanvas.h"
#include "wz/WzProperty.h"
#include "wz/WzResMan.h"

namespace ms
{

LayoutMan::LayoutMan() = default;

LayoutMan::~LayoutMan() = default;

void LayoutMan::Init(UIElement* pParent, int nOffsetX, int nOffsetY)
{
    m_pParent = pParent;
    m_nOffsetX = nOffsetX;
    m_nOffsetY = nOffsetY;
}

void LayoutMan::AutoBuild(
    const std::wstring& sRootUOL,
    int nIdBase,
    int nOffsetX,
    int nOffsetY,
    bool bSetTooltip,
    bool bSameIDCtrl)
{
    // IDA: 從 WzResMan 獲取根屬性 (0xb362a7-0xb36330)
    auto& resMan = WzResMan::GetInstance();

    // 轉換 wstring 到 string
    std::string sRootPath;
    sRootPath.reserve(sRootUOL.size());
    for (wchar_t wc : sRootUOL)
    {
        if (wc > 127)
        {
            // 非 ASCII 字符
            LOG_WARN("LayoutMan::AutoBuild - non-ASCII character in UOL");
            return;
        }
        sRootPath.push_back(static_cast<char>(wc));
    }

    LOG_DEBUG("LayoutMan::AutoBuild - trying to load: {}", sRootPath);

    auto pRoot = resMan.GetProperty(sRootPath);
    if (!pRoot)
    {
        LOG_WARN("LayoutMan::AutoBuild - property not found: {}", sRootPath);
        return;
    }

    LOG_DEBUG("LayoutMan::AutoBuild - found root property with {} children", pRoot->GetChildren().size());

    // IDA: 枚舉所有子屬性 (0xb36435-0xb364d3)
    int currentId = 0;  // Start offset at 0

    for (auto& [name, pProp] : pRoot->GetChildren())
    {
        // 轉換 string 到 wstring
        std::wstring wName;
        wName.reserve(name.size());
        for (char c : name)
        {
            wName.push_back(static_cast<wchar_t>(c));
        }

        LOG_DEBUG("LayoutMan::AutoBuild - processing child: {}", name);

        // TODO: 下一步處理
        ProcessChildProperty(wName, pProp, sRootUOL, nIdBase, currentId,
                           nOffsetX, nOffsetY, bSetTooltip, bSameIDCtrl);
    }
}

void LayoutMan::ProcessChildProperty(
    const std::wstring& wName,
    std::shared_ptr<WzProperty> pProp,
    const std::wstring& sRootUOL,
    int nIdBase,
    int& currentId,
    int nOffsetX,
    int nOffsetY,
    bool bSetTooltip,
    bool bSameIDCtrl)
{
    // IDA: 查找冒號 (0xb36553-0xb3655f)
    auto colonPos = wName.find(L':');
    if (colonPos == std::wstring::npos)
    {
        // 沒有冒號，跳過
        return;
    }

    // IDA: 分割 type 和 name (0xb365a8-0xb365b2)
    std::wstring sType = wName.substr(0, colonPos);       // "button"
    std::wstring sCtrlName = wName.substr(colonPos + 1);  // "GoWorld"

    // IDA: 比較類型字符串 (0xb365ed)
    if (sType == L"button")
    {
        ProcessButton(wName, sCtrlName, pProp, sRootUOL, nIdBase, currentId,
                     nOffsetX, nOffsetY, bSameIDCtrl);
    }
    else if (sType == L"layer")
    {
        ProcessLayer(wName, sCtrlName, pProp, sRootUOL, nOffsetX, nOffsetY);
    }
    // 其他類型...
}

void LayoutMan::ProcessButton(
    const std::wstring& wFullName,
    const std::wstring& sCtrlName,
    std::shared_ptr<WzProperty> pProp,
    const std::wstring& sRootUOL,
    int nIdBase,
    int& currentId,
    int nOffsetX,
    int nOffsetY,
    bool bSameIDCtrl)
{
    // IDA: 讀取配置參數

    // 1. 讀取 "id" (0xb36707-0xb36755)
    // 讀取可選的 ID 偏移量（默認使用自動遞增）
    int nIDOffset = currentId;
    auto pIdProp = pProp->GetChild("id");
    if (pIdProp)
    {
        nIDOffset = pIdProp->GetInt(0);  // 如果指定了 id，使用指定值
    }

    // 最終 ID = 基礎 ID + 偏移量
    int nID = nIdBase + nIDOffset;
    currentId++;  // 自動遞增

    // 2. 讀取 "drawBack" (0xb3679f-0xb367e6)
    // TODO: drawBack 參數的具體用途待確認（可能控制繪製順序）
    // bool bDrawBack = false;
    // auto pDrawBackProp = pProp->GetChild("drawBack");
    // if (pDrawBackProp)
    // {
    //     bDrawBack = pDrawBackProp->GetInt(0) != 0;
    // }

    // 3. 讀取 "toggle" (0xb3687c-0xb368de)
    bool bToggle = false;
    auto pToggleProp = pProp->GetChild("toggle");
    if (pToggleProp)
    {
        bToggle = pToggleProp->GetInt(0) != 0;
    }

    // 4. 構建完整 UOL 路徑 (0xb368b0-0xb368ce)
    std::wstring sButtonUOL = sRootUOL + L"/" + sCtrlName;

    // 5. 調用 AddButton (0xb36908)
    auto pButton = AddButton(sButtonUOL, nID, nOffsetX, nOffsetY, bToggle, bSameIDCtrl);
    if (!pButton)
    {
        return;
    }

    // 6. 讀取並設置 "enable" (0xb369c4-0xb36a18)
    bool bEnable = true;
    auto pEnableProp = pProp->GetChild("enable");
    if (pEnableProp)
    {
        bEnable = pEnableProp->GetInt(1) != 0;
    }
    pButton->SetEnabled(bEnable);

    // 7. 讀取並設置 "visible" (0xb36a63-0xb36ab0)
    bool bVisible = true;
    auto pVisibleProp = pProp->GetChild("visible");
    if (pVisibleProp)
    {
        bVisible = pVisibleProp->GetInt(1) != 0;
    }
    pButton->SetVisible(bVisible);

    // 8. 註冊到映射表
    m_mButtons[sCtrlName] = pButton;
}

void LayoutMan::ProcessLayer(
    const std::wstring& wFullName,
    const std::wstring& sCtrlName,
    std::shared_ptr<WzProperty> pProp,
    const std::wstring& sRootUOL,
    int nOffsetX,
    int nOffsetY)
{
    // 1. 獲取 canvas
    auto pCanvas = pProp->GetCanvas();
    if (!pCanvas)
    {
        return;
    }

    // 2. 獲取 origin
    auto origin = pCanvas->GetOrigin();

    // 3. 創建圖層
    if (!m_pParent)
    {
        return;
    }

    // TODO: 需要從 m_pParent 獲取 WzGr2D 實例
    // auto& gr = m_pParent->GetGraphics();
    // auto pLayer = gr.CreateLayer(
    //     nOffsetX + m_nOffsetX + origin.x,
    //     nOffsetY + m_nOffsetY + origin.y,
    //     pCanvas->GetWidth(),
    //     pCanvas->GetHeight(),
    //     100  // z-order
    // );

    // 4. 插入 canvas
    // pLayer->InsertCanvas(pCanvas, 0, 255, 255);

    // 5. 註冊圖層
    // RegisterLayer(pLayer, sCtrlName);
}

auto LayoutMan::ABGetButton(const std::wstring& sName) -> std::shared_ptr<UIButton>
{
    // IDA: ZMap::GetPos
    auto it = m_mButtons.find(sName);
    if (it != m_mButtons.end())
    {
        return it->second;
    }

    // IDA: 返回 static empty reference
    return nullptr;
}

void LayoutMan::ABSetButtonShowAll(bool bShow)
{
    // IDA: 遍歷 m_mButtons 映射表
    for (auto& [name, pButton] : m_mButtons)
    {
        if (pButton)
        {
            pButton->SetVisible(bShow);
        }
    }
}

void LayoutMan::ABSetButtonEnableAll(bool bEnable)
{
    // IDA: 遍歷 m_mButtons 映射表
    for (auto& [name, pButton] : m_mButtons)
    {
        if (pButton)
        {
            pButton->SetEnabled(bEnable);
        }
    }
}

void LayoutMan::ABSetButtonEnable(const std::wstring& sName, bool bEnable)
{
    // IDA: 先查找按鈕
    auto pButton = ABGetButton(sName);
    if (pButton)
    {
        pButton->SetEnabled(bEnable);
    }
}

void LayoutMan::RegisterLayer(std::shared_ptr<WzGr2DLayer> pLayer, const std::wstring& sName)
{
    if (!pLayer)
    {
        return;
    }

    // IDA: 添加到圖層陣列
    m_aLayer.push_back(pLayer);

    // IDA: 如果提供名稱，註冊到映射表
    if (!sName.empty())
    {
        m_mLayers[sName] = pLayer;
    }
}

auto LayoutMan::ABGetLayer(const std::wstring& sName) -> std::shared_ptr<WzGr2DLayer>
{
    // IDA: ZMap::GetPos
    auto it = m_mLayers.find(sName);
    if (it != m_mLayers.end())
    {
        return it->second;
    }

    // IDA: 返回 static empty reference
    return nullptr;
}

void LayoutMan::ABSetLayerVisible(const std::wstring& sName, bool bVisible)
{
    // IDA: 先查找圖層
    auto pLayer = ABGetLayer(sName);
    if (pLayer)
    {
        pLayer->SetVisible(bVisible);
    }
}

void LayoutMan::ABSetLayerVisibleAll(bool bVisible)
{
    // IDA: 遍歷 m_mLayers 映射表
    for (auto& [name, pLayer] : m_mLayers)
    {
        if (pLayer)
        {
            pLayer->SetVisible(bVisible);
        }
    }
}

auto LayoutMan::AddButton(
    const std::wstring& sButtonUOL,
    unsigned int nID,
    int nOffsetX,
    int nOffsetY,
    bool bToggle,
    bool bSkipIDCheck) -> std::shared_ptr<UIButton>
{
    // 轉換 wstring 到 string for logging
    std::string sPath;
    for (wchar_t wc : sButtonUOL)
    {
        if (wc <= 127)
        {
            sPath.push_back(static_cast<char>(wc));
        }
    }
    LOG_DEBUG("LayoutMan::AddButton - trying to load button from: {}", sPath);

    // IDA: 創建按鈕對象 (0xb30424)
    auto pButton = std::make_shared<UIButton>();

    // IDA: 從 UOL 路徑加載資源
    if (!pButton->LoadFromUOL(sButtonUOL))
    {
        LOG_WARN("LayoutMan::AddButton - failed to load button from: {}", sPath);
        return nullptr;
    }

    LOG_DEBUG("LayoutMan::AddButton - successfully loaded button from: {}", sPath);

    // IDA: 設置 toggle 模式
    if (bToggle)
    {
        pButton->SetCheckMode(true);
    }

    // IDA: 設置位置 (0xb3048b-0xb30498)
    // 使用全局偏移 + 局部偏移
    pButton->SetPosition(nOffsetX + m_nOffsetX, nOffsetY + m_nOffsetY);

    // IDA: 設置父元素
    if (m_pParent)
    {
        pButton->SetParent(m_pParent);
    }

    // IDA: 設置控件 ID (0xb30499)
    pButton->SetID(nID);

    // IDA: ID 檢查和替換邏輯 (0xb304a1-0xb304d0)
    if (!bSkipIDCheck && nID != 0)
    {
        // 檢查是否已存在相同 ID 的控件
        for (size_t i = 0; i < m_aCtrl.size(); ++i)
        {
            if (m_aCtrl[i] && m_aCtrl[i]->GetID() == nID)
            {
                // 替換現有控件
                LOG_DEBUG("LayoutMan::AddButton - replacing existing control with ID {}", nID);
                m_aCtrl[i] = pButton;
                return pButton;
            }
        }
    }

    // IDA: 添加到控件陣列 (0xb305a1-0xb30696)
    m_aCtrl.push_back(pButton);

    return pButton;
}

} // namespace ms

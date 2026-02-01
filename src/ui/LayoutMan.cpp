#include "LayoutMan.h"
#include "UIButton.h"
#include "UIElement.h"
#include "graphics/WzGr2DLayer.h"
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
    // TODO: Task 4
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
    // TODO: Task 4
    return nullptr;
}

} // namespace ms

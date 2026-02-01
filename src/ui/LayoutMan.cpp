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
    // TODO: Task 3
}

void LayoutMan::ABSetButtonEnableAll(bool bEnable)
{
    // TODO: Task 3
}

void LayoutMan::ABSetButtonEnable(const std::wstring& sName, bool bEnable)
{
    // TODO: Task 3
}

void LayoutMan::RegisterLayer(std::shared_ptr<WzGr2DLayer> pLayer, const std::wstring& sName)
{
    // TODO: Task 3
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
    // TODO: Task 3
}

void LayoutMan::ABSetLayerVisibleAll(bool bVisible)
{
    // TODO: Task 3
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

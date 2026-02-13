#include "VecCtrl.h"

#include "app/Application.h"

namespace ms
{

VecCtrl::VecCtrl() = default;

// =============================================================================
// IWzShape2D — CVecCtrl manages its own position via AbsPosEx
// =============================================================================

// CVecCtrl::get_x — calls GetSnapshot to get interpolated x
auto VecCtrl::GetX() -> std::int32_t
{
    std::int32_t x{};
    GetSnapshot(&x, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    return x;
}

// CVecCtrl::get_y — calls GetSnapshot to get interpolated y
auto VecCtrl::GetY() -> std::int32_t
{
    std::int32_t y{};
    GetSnapshot(nullptr, &y, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    return y;
}

// CVecCtrl::put_x — SetApToApl then Move(newX, currentY)
void VecCtrl::PutX(std::int32_t x)
{
    SetApToApl();
    Move(x, static_cast<std::int32_t>(m_ap.y));
}

// CVecCtrl::put_y — SetApToApl then Move(currentX, newY)
void VecCtrl::PutY(std::int32_t y)
{
    SetApToApl();
    Move(static_cast<std::int32_t>(m_ap.x), y);
}

// CVecCtrl::raw_Move — set absolute position, zero velocities
void VecCtrl::Move(std::int32_t x, std::int32_t y)
{
    auto xd = static_cast<long double>(x);
    auto yd = static_cast<long double>(y);

    // Set current and last position (instant teleport)
    m_ap.x = xd;
    m_ap.y = yd;
    m_apl.x = xd;
    m_apl.y = yd;

    // Zero velocities
    m_ap.vx = 0.0L;
    m_ap.vy = 0.0L;
    m_apl.vx = 0.0L;
    m_apl.vy = 0.0L;

    // TODO: if (m_pOwner) SetMovePathAttribute(3, 0);
}

// CVecCtrl::raw_Offset — offset current position then Move
void VecCtrl::Offset(std::int32_t dx, std::int32_t dy)
{
    Move(static_cast<std::int32_t>(m_ap.x + static_cast<long double>(dx)),
         static_cast<std::int32_t>(m_ap.y + static_cast<long double>(dy)));
}

// CVecCtrl::raw_Scale — scale position around center then Move
void VecCtrl::Scale(std::int32_t sx, std::int32_t divx,
                    std::int32_t sy, std::int32_t divy,
                    std::int32_t cx, std::int32_t cy)
{
    if (divx == 0 || divy == 0)
        return;

    auto cxd = static_cast<double>(cx);
    auto cyd = static_cast<double>(cy);
    auto newX = static_cast<std::int32_t>(
        cxd + (m_ap.x - cxd) * static_cast<double>(sx) / static_cast<double>(divx));
    auto newY = static_cast<std::int32_t>(
        cyd + (m_ap.y - cyd) * static_cast<double>(sy) / static_cast<double>(divy));

    Move(newX, newY);
}

// CVecCtrl::raw_Init — delegates to Move
void VecCtrl::Init(std::int32_t x, std::int32_t y)
{
    Move(x, y);
}

// =============================================================================
// IWzVector2D — Most return E_NOTIMPL (not supported by CVecCtrl)
// =============================================================================

// CVecCtrl::get_currentTime — E_NOTIMPL
auto VecCtrl::GetCurrentTime() -> std::int32_t { return 0; }

// CVecCtrl::put_currentTime — E_NOTIMPL
void VecCtrl::PutCurrentTime(std::int32_t /*t*/) {}

// CVecCtrl::get_origin — always returns nullptr
auto VecCtrl::GetOrigin() const -> IWzVector2D* { return nullptr; }

// CVecCtrl::put_origin — E_NOTIMPL
void VecCtrl::PutOrigin(IWzVector2D* /*parent*/) {}

// CVecCtrl::get_rx — calls GetSnapshot to get interpolated rx
auto VecCtrl::GetRX() -> std::int32_t
{
    std::int32_t rx{};
    GetSnapshot(nullptr, nullptr, &rx, nullptr, nullptr, nullptr, nullptr, nullptr);
    return rx;
}

// CVecCtrl::put_rx — SetApToApl then RelMove(x, currentY)
void VecCtrl::PutRX(std::int32_t x)
{
    SetApToApl();
    Move(x, static_cast<std::int32_t>(m_ap.y));
}

// CVecCtrl::get_ry — calls GetSnapshot to get interpolated ry
auto VecCtrl::GetRY() -> std::int32_t
{
    std::int32_t ry{};
    GetSnapshot(nullptr, nullptr, nullptr, &ry, nullptr, nullptr, nullptr, nullptr);
    return ry;
}

// CVecCtrl::put_ry — SetApToApl then RelMove(currentX, y)
void VecCtrl::PutRY(std::int32_t y)
{
    SetApToApl();
    Move(static_cast<std::int32_t>(m_ap.x), y);
}

// CVecCtrl::get_a — E_NOTIMPL
auto VecCtrl::GetA() -> double { return 0.0; }

// CVecCtrl::get_ra — E_NOTIMPL
auto VecCtrl::GetRA() -> double { return 0.0; }

// CVecCtrl::put_ra — E_NOTIMPL
void VecCtrl::PutRA(double /*a*/) {}

// CVecCtrl::get_flipX — always 0
auto VecCtrl::GetFlipX() -> bool { return false; }

// CVecCtrl::put_flipX — no-op (S_OK)
void VecCtrl::PutFlipX(std::int32_t /*f*/) {}

// CVecCtrl::raw__GetSnapshot — interpolates m_apl → m_ap by frame time,
// or delegates to m_pVecAlternate if set.
// Origin offsets (ox,oy) and angles (a,ra) are always 0.
void VecCtrl::GetSnapshot(std::int32_t* x, std::int32_t* y,
                          std::int32_t* rx, std::int32_t* ry,
                          std::int32_t* ox, std::int32_t* oy,
                          double* a, double* ra,
                          std::int32_t time)
{
    if (m_pVecAlternate)
    {
        // Delegate to alternate vector
        m_pVecAlternate->GetSnapshot(x, y, rx, ry, nullptr, nullptr, nullptr, nullptr, time);
    }
    else
    {
        // Interpolate between last (m_apl) and current (m_ap) based on frame time
        // Original: t = CWvsApp::GetTimeGap() / 30.0
        long double t = static_cast<long double>(
            Application::GetInstance().GetTimeGap()) / 30.0L;

        auto Interp = [t](long double last, long double cur) -> std::int32_t {
            long double val = last + (cur - last) * t;
            return val >= 0.0L
                ? static_cast<std::int32_t>(val + 0.5L)
                : static_cast<std::int32_t>(val - 0.5L + 1e-9L);
        };

        if (x)  *x  = Interp(m_apl.x, m_ap.x);
        if (y)  *y  = Interp(m_apl.y, m_ap.y);
        if (rx) *rx = Interp(m_apl.x, m_ap.x);
        if (ry) *ry = Interp(m_apl.y, m_ap.y);
    }

    // Origin offsets and angles are always 0 for CVecCtrl
    if (ox) *ox = 0;
    if (oy) *oy = 0;
    if (a)  *a  = 0.0;
    if (ra) *ra = 0.0;
}

// CVecCtrl::raw_RelMove — delegates to Move (ignores timing)
void VecCtrl::RelMove(std::int32_t x, std::int32_t y,
                      std::int32_t /*startTime*/, std::int32_t /*endTime*/,
                      bool /*bounce*/, bool /*pingpong*/, bool /*replace*/)
{
    Move(x, y);
}

// CVecCtrl::raw_RelOffset — delegates to Offset (ignores timing)
void VecCtrl::RelOffset(std::int32_t dx, std::int32_t dy,
                        std::int32_t /*startTime*/, std::int32_t /*endTime*/)
{
    Offset(dx, dy);
}

// CVecCtrl::raw_Ratio — E_NOTIMPL
void VecCtrl::Ratio(IWzVector2D* /*target*/,
                    std::int32_t /*denomX*/, std::int32_t /*denomY*/,
                    std::int32_t /*scaleX*/, std::int32_t /*scaleY*/)
{
}

// CVecCtrl::raw_WrapClip — E_NOTIMPL
void VecCtrl::WrapClip(IWzVector2D* /*bounds*/,
                       std::int32_t /*x*/, std::int32_t /*y*/,
                       std::int32_t /*w*/, std::int32_t /*h*/,
                       bool /*clampMode*/)
{
}

// CVecCtrl::raw_Rotate — E_NOTIMPL
void VecCtrl::Rotate(double /*angle*/, std::int32_t /*period*/,
                     std::int32_t /*easeFrames*/)
{
}

// CVecCtrl::get_looseLevel — E_NOTIMPL
auto VecCtrl::GetLooseLevel() -> std::int32_t { return 0; }

// CVecCtrl::put_looseLevel — E_NOTIMPL
void VecCtrl::PutLooseLevel(std::int32_t /*level*/) {}

// CVecCtrl::raw_Fly — returns E_FAIL (not supported)
void VecCtrl::Fly(const std::vector<FlyKeyframe>& /*keyframes*/,
                  IWzVector2D* /*completionTarget*/)
{
}

} // namespace ms

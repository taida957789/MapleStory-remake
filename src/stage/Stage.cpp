#include "Stage.h"

namespace ms
{

Stage* g_pStage = nullptr;

Stage::Stage() = default;

Stage::~Stage() = default;

void Stage::Init([[maybe_unused]] void* param)
{
    // Base implementation does nothing
    // Derived classes override this to perform initialization
}

void Stage::Close()
{
    // Base implementation does nothing
    // Derived classes override this to perform cleanup
}

auto Stage::OnSetFocus([[maybe_unused]] int focused) -> bool
{
    // Base implementation returns success
    return true;
}

void Stage::OnMouseMove([[maybe_unused]] std::int32_t x, [[maybe_unused]] std::int32_t y)
{
    // Base implementation does nothing
}

void Stage::OnMouseDown([[maybe_unused]] std::int32_t x, [[maybe_unused]] std::int32_t y,
                        [[maybe_unused]] std::int32_t button)
{
    // Base implementation does nothing
}

void Stage::OnMouseUp([[maybe_unused]] std::int32_t x, [[maybe_unused]] std::int32_t y,
                      [[maybe_unused]] std::int32_t button)
{
    // Base implementation does nothing
}

void Stage::OnKeyDown([[maybe_unused]] std::int32_t keyCode)
{
    // Base implementation does nothing
}

void Stage::OnKeyUp([[maybe_unused]] std::int32_t keyCode)
{
    // Base implementation does nothing
}

void Stage::OnTextInput([[maybe_unused]] const std::string& text)
{
    // Base implementation does nothing
}

} // namespace ms

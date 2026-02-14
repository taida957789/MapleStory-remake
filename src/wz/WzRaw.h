#pragma once

#include "WzTypes.h"

#include <cstdint>
#include <vector>

namespace ms
{

class WzRaw
{
public:
    WzRaw();
    ~WzRaw();

    // Non-copyable, movable
    WzRaw(const WzRaw&) = delete;
    auto operator=(const WzRaw&) -> WzRaw& = delete;
    WzRaw(WzRaw&& other) noexcept;
    auto operator=(WzRaw&& other) noexcept -> WzRaw&;

    void SetType(std::int32_t t) { m_nType = t; }
    void SetData(const std::vector<std::uint8_t>& d) { m_data = d; }

    [[nodiscard]] auto GetType() const noexcept -> std::int32_t { return m_nType; }
    [[nodiscard]] auto GetData() const -> const std::vector<std::uint8_t>& { return m_data; }

private:
    std::int32_t m_nType{};
    std::vector<std::uint8_t> m_data;
};

} // namespace ms

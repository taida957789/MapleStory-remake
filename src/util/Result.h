#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <utility>

// Use fmt library for formatting (included via spdlog)
#include <spdlog/fmt/bundled/format.h>

namespace ms
{

/**
 * @brief Modern C++ error handling type
 *
 * Result<T> represents either a successful value of type T or an error message.
 * Forces explicit error checking via [[nodiscard]] attribute.
 *
 * Usage:
 *   auto result = SomeFunction();
 *   if (!result) {
 *       LOG_ERROR("Failed: {}", result.error());
 *       return;
 *   }
 *   auto value = result.value();
 */
template<typename T>
class [[nodiscard]] Result
{
public:
    // Success construction
    static auto Success(T value = T{}) -> Result
    {
        return Result(std::move(value));
    }

    // Error construction with formatting
    template<typename... Args>
    static auto Error(fmt::format_string<Args...> fmt, Args&&... args) -> Result
    {
        return Result(fmt::format(fmt, std::forward<Args>(args)...));
    }

    // Check if successful
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return m_value.has_value();
    }

    // Get value (throws if error)
    [[nodiscard]] auto value() const -> const T&
    {
        if (!m_value)
        {
            throw std::runtime_error(m_error);
        }
        return *m_value;
    }

    // Get value (throws if error)
    [[nodiscard]] auto value() -> T&
    {
        if (!m_value)
        {
            throw std::runtime_error(m_error);
        }
        return *m_value;
    }

    // Get error message
    [[nodiscard]] auto error() const noexcept -> std::string_view
    {
        return m_error;
    }

private:
    explicit Result(T value) : m_value(std::move(value)) {}
    explicit Result(std::string error) : m_error(std::move(error)) {}

    std::optional<T> m_value;
    std::string m_error;
};

// Specialization for void (success/failure only)
template<>
class [[nodiscard]] Result<void>
{
public:
    static auto Success() -> Result { return Result(true); }

    template<typename... Args>
    static auto Error(fmt::format_string<Args...> fmt, Args&&... args) -> Result
    {
        return Result(fmt::format(fmt, std::forward<Args>(args)...));
    }

    [[nodiscard]] explicit operator bool() const noexcept { return m_success; }
    [[nodiscard]] auto error() const noexcept -> std::string_view { return m_error; }

private:
    explicit Result(bool success) : m_success(success) {}
    explicit Result(std::string error) : m_success(false), m_error(std::move(error)) {}

    bool m_success{false};
    std::string m_error;
};

} // namespace ms

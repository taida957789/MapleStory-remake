#pragma once

namespace ms
{

/**
 * @brief Thread-safe Singleton pattern using Meyers' Singleton
 *
 * Based on TSingleton<T> from the original MapleStory client.
 * Uses C++11 magic statics for thread-safe initialization.
 *
 * @tparam T The derived singleton class
 */
template <typename T>
class Singleton
{
public:
    /**
     * @brief Get the singleton instance
     * @return Reference to the singleton instance
     */
    [[nodiscard]] static auto GetInstance() noexcept -> T&
    {
        static T instance;
        return instance;
    }

    /**
     * @brief Get pointer to the singleton instance
     * @return Pointer to the singleton instance
     */
    [[nodiscard]] static auto GetInstancePtr() noexcept -> T*
    {
        return &GetInstance();
    }

    /**
     * @brief Check if instance is created (always true with static storage)
     * @return Always returns true
     */
    [[nodiscard]] static constexpr auto IsInstantiated() noexcept -> bool
    {
        return true;
    }

protected:
    Singleton() = default;
    virtual ~Singleton() = default;

    // Non-copyable, non-movable
    Singleton(const Singleton&) = delete;
    auto operator=(const Singleton&) -> Singleton& = delete;
    Singleton(Singleton&&) = delete;
    auto operator=(Singleton&&) -> Singleton& = delete;
};

} // namespace ms

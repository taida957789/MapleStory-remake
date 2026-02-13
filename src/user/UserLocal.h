#pragma once

#include "user/User.h"
#include "util/Singleton.h"

#include <cstdint>

namespace ms
{

/**
 * @brief Local player character state
 *
 * Based on CUserLocal (__cppobj : CUser) from the original MapleStory client.
 * Singleton that represents the locally-controlled player character.
 */
class UserLocal final : public User, public Singleton<UserLocal>
{
    friend class Singleton<UserLocal>;

public:
    /// Backward-compatible accessor (UserLocal IS-A Avatar via User).
    [[nodiscard]] auto GetAvatar() noexcept -> Avatar& { return *this; }
    [[nodiscard]] auto GetAvatar() const noexcept -> const Avatar& { return *this; }

    [[nodiscard]] auto GetMoveAction() const noexcept -> std::int32_t { return m_nMoveAction; }

private:
    UserLocal();
    ~UserLocal() override;
};

} // namespace ms

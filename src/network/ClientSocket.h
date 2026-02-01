#pragma once

#include "util/Singleton.h"
#include <cstdint>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

namespace ms
{

class InPacket;
class OutPacket;

/**
 * Client socket for network communication
 * Based on CClientSocket from the original client
 *
 * Handles:
 * - Connection to login/game servers
 * - Packet encryption/decryption
 * - Send/receive queuing
 */
class ClientSocket : public Singleton<ClientSocket>
{
    friend class Singleton<ClientSocket>;

public:
    [[nodiscard]] auto Initialize() -> bool;
    void Shutdown();

    /**
     * Connect to server
     * Based on CClientSocket::Connect
     */
    [[nodiscard]] auto Connect(const std::string& host, std::uint16_t port) -> bool;

    /**
     * Disconnect from server
     */
    void Disconnect();

    /**
     * Check if connected
     */
    [[nodiscard]] auto IsConnected() const noexcept -> bool { return m_bConnected; }

    /**
     * Send packet
     * Based on CClientSocket::SendPacket
     */
    void SendPacket(const OutPacket& packet);

    /**
     * Process received packets
     * Based on CClientSocket::ManipulatePacket
     */
    void ManipulatePacket();

    /**
     * Run delayed packet processing
     * Based on CClientSocket::RunDelayedProcessPacket
     */
    void RunDelayedProcessPacket();

    /**
     * Apply hotfix settings
     * Based on CClientSocket::ApplyHotfix
     */
    void ApplyHotfix();

private:
    ClientSocket();
    ~ClientSocket();

    // Non-copyable, non-movable (singleton)
    ClientSocket(const ClientSocket&) = delete;
    auto operator=(const ClientSocket&) -> ClientSocket& = delete;
    ClientSocket(ClientSocket&&) = delete;
    auto operator=(ClientSocket&&) -> ClientSocket& = delete;

    // Socket handle (would be SOCKET on Windows)
    int m_socket{-1};

    // Connection state
    bool m_bConnected{false};

    // Server info
    std::string m_sHost;
    std::uint16_t m_nPort{};

    // Packet queues
    std::queue<std::vector<std::uint8_t>> m_sendQueue;
    std::queue<std::vector<std::uint8_t>> m_recvQueue;
    std::mutex m_sendMutex;
    std::mutex m_recvMutex;

    // Encryption keys (AES)
    std::uint32_t m_dwSendIV{};
    std::uint32_t m_dwRecvIV{};
    std::uint16_t m_nVersion{};
};

} // namespace ms

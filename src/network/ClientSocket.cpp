#include "ClientSocket.h"
#include "InPacket.h"
#include "OutPacket.h"
#include "util/Logger.h"

namespace ms
{

ClientSocket::ClientSocket() = default;

ClientSocket::~ClientSocket()
{
    Shutdown();
}

auto ClientSocket::Initialize() -> bool
{
    // Initialize networking
    // On Windows would call WSAStartup
    return true;
}

void ClientSocket::Shutdown()
{
    Disconnect();
}

auto ClientSocket::Connect(const std::string& host, std::uint16_t port) -> bool
{
    // TODO: Implement actual socket connection
    m_sHost = host;
    m_nPort = port;

    LOG_INFO("Connecting to {}:{}", host, port);

    // For now, just mark as "connected" for testing
    m_bConnected = true;
    return true;
}

void ClientSocket::Disconnect()
{
    if (!m_bConnected)
        return;

    // TODO: Close actual socket

    m_bConnected = false;
    LOG_INFO("Disconnected from server");
}

void ClientSocket::SendPacket([[maybe_unused]] const OutPacket& packet)
{
    if (!m_bConnected)
        return;

    // TODO: Encrypt and send packet
    std::lock_guard<std::mutex> lock(m_sendMutex);
    // m_sendQueue.push(packet.GetData());
}

void ClientSocket::ManipulatePacket()
{
    // Process incoming packets
    std::lock_guard<std::mutex> lock(m_recvMutex);

    while (!m_recvQueue.empty())
    {
        [[maybe_unused]] auto& data = m_recvQueue.front();
        // TODO: Decrypt and process packet
        m_recvQueue.pop();
    }
}

void ClientSocket::RunDelayedProcessPacket()
{
    // Process any delayed packets
}

void ClientSocket::ApplyHotfix()
{
    // Apply any hotfix settings from server
}

} // namespace ms

#pragma once

#include <cstdint>

namespace Spine
{

// Reconstructed from MapleStory binary (@ 0x1baa1d0).
// Simple non-owning byte stream over a buffer with sequential read position.
class BufferedStream
{
public:
    BufferedStream(void* pBuffer, unsigned int nLength)
        : m_pBuffer(static_cast<std::uint8_t*>(pBuffer)),
          m_nLength(nLength),
          m_nPosition(0)
    {
    }

    auto ReadByte() -> std::uint8_t
    {
        if (m_nPosition >= m_nLength)
            return 0xFF;
        return m_pBuffer[m_nPosition++];
    }

    auto GetBuffer() const -> const std::uint8_t* { return m_pBuffer; }
    auto GetLength() const -> unsigned int { return m_nLength; }
    auto GetPosition() const -> unsigned int { return m_nPosition; }
    void SetPosition(unsigned int nPos) { m_nPosition = nPos; }

private:
    std::uint8_t* m_pBuffer;
    unsigned int m_nLength;
    unsigned int m_nPosition;
};

} // namespace Spine

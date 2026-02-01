#pragma once

#include "util/Singleton.h"
#include "wz/WzCanvas.h"

#include <cstdint>
#include <memory>
#include <string>

// Forward declarations for FreeType
typedef struct FT_LibraryRec_* FT_Library;
typedef struct FT_FaceRec_* FT_Face;

struct SDL_Renderer;

namespace ms
{

/**
 * @brief Text rendering using FreeType
 *
 * Singleton class that handles text rendering using FreeType library.
 * Based on sdlms FreeType implementation.
 */
class TextRenderer final : public Singleton<TextRenderer>
{
    friend class Singleton<TextRenderer>;

public:
    ~TextRenderer();

    /**
     * @brief Initialize FreeType and load default font
     * @param fontPath Path to font file (uses system font if empty)
     * @return true if initialization succeeded
     */
    auto Initialize(const std::string& fontPath = "") -> bool;

    /**
     * @brief Shutdown FreeType
     */
    void Shutdown();

    /**
     * @brief Set font size in pixels
     * @param size Font size in pixels
     */
    void SetFontSize(std::int32_t size);

    /**
     * @brief Render text to a WzCanvas
     * @param text Text to render (UTF-8)
     * @param color Text color (RGBA)
     * @return Canvas containing rendered text, or nullptr on failure
     */
    auto RenderText(const std::string& text, std::uint32_t color = 0xFF000000)
        -> std::shared_ptr<WzCanvas>;

    /**
     * @brief Render text directly to SDL renderer
     * @param renderer SDL renderer
     * @param text Text to render (UTF-8)
     * @param x X position
     * @param y Y position
     * @param color Text color (RGBA)
     */
    void RenderTextDirect(SDL_Renderer* renderer, const std::string& text,
                          std::int32_t x, std::int32_t y,
                          std::uint32_t color = 0xFF000000);

    /**
     * @brief Check if TextRenderer is initialized
     */
    [[nodiscard]] auto IsInitialized() const noexcept -> bool { return m_bInitialized; }

    /**
     * @brief Get current font size
     */
    [[nodiscard]] auto GetFontSize() const noexcept -> std::int32_t { return m_nFontSize; }

private:
    TextRenderer();

    // Convert UTF-8 string to UTF-32 for FreeType
    static auto Utf8ToUtf32(const std::string& utf8) -> std::u32string;

    FT_Library m_pLibrary{nullptr};
    FT_Face m_pFace{nullptr};
    bool m_bInitialized{false};
    std::int32_t m_nFontSize{12};
};

} // namespace ms

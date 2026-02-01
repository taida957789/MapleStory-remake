#include "TextRenderer.h"
#include "util/Logger.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <SDL3/SDL.h>
#include <algorithm>
#include <codecvt>
#include <locale>

namespace ms
{

TextRenderer::TextRenderer() = default;

TextRenderer::~TextRenderer()
{
    Shutdown();
}

auto TextRenderer::Initialize(const std::string& fontPath) -> bool
{
    if (m_bInitialized)
    {
        return true;
    }

    // Initialize FreeType library
    FT_Error error = FT_Init_FreeType(&m_pLibrary);
    if (error)
    {
        LOG_ERROR("Failed to initialize FreeType library: {}", error);
        return false;
    }

    // Try to load font
    std::string actualFontPath = fontPath;
    if (actualFontPath.empty())
    {
        // Try common font locations
#ifdef _WIN32
        actualFontPath = "C:/Windows/Fonts/simsun.ttc";
#elif __APPLE__
        actualFontPath = "/System/Library/Fonts/PingFang.ttc";
#else
        // Linux - try common locations
        const char* fontPaths[] = {
            "/usr/share/fonts/truetype/droid/DroidSansFallbackFull.ttf",
            "/usr/share/fonts/truetype/wqy/wqy-microhei.ttc",
            "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/TTF/DejaVuSans.ttf",
            nullptr
        };
        for (const char** path = fontPaths; *path; ++path)
        {
            if (std::ifstream(*path).good())
            {
                actualFontPath = *path;
                break;
            }
        }
#endif
    }

    if (actualFontPath.empty())
    {
        LOG_ERROR("No font file found");
        FT_Done_FreeType(m_pLibrary);
        m_pLibrary = nullptr;
        return false;
    }

    error = FT_New_Face(m_pLibrary, actualFontPath.c_str(), 0, &m_pFace);
    if (error)
    {
        LOG_ERROR("Failed to load font '{}': {}", actualFontPath, error);
        FT_Done_FreeType(m_pLibrary);
        m_pLibrary = nullptr;
        return false;
    }

    // Select Unicode charmap
    FT_Select_Charmap(m_pFace, FT_ENCODING_UNICODE);

    // Set default font size
    SetFontSize(m_nFontSize);

    m_bInitialized = true;
    LOG_INFO("TextRenderer initialized with font: {}", actualFontPath);
    return true;
}

void TextRenderer::Shutdown()
{
    if (m_pFace)
    {
        FT_Done_Face(m_pFace);
        m_pFace = nullptr;
    }
    if (m_pLibrary)
    {
        FT_Done_FreeType(m_pLibrary);
        m_pLibrary = nullptr;
    }
    m_bInitialized = false;
}

void TextRenderer::SetFontSize(std::int32_t size)
{
    m_nFontSize = size;
    if (m_pFace)
    {
        FT_Set_Pixel_Sizes(m_pFace, 0, static_cast<FT_UInt>(size));
    }
}

auto TextRenderer::Utf8ToUtf32(const std::string& utf8) -> std::u32string
{
    std::u32string result;
    result.reserve(utf8.size());

    size_t i = 0;
    while (i < utf8.size())
    {
        char32_t codepoint = 0;
        unsigned char c = static_cast<unsigned char>(utf8[i]);

        if ((c & 0x80) == 0)
        {
            // ASCII
            codepoint = c;
            i += 1;
        }
        else if ((c & 0xE0) == 0xC0)
        {
            // 2-byte sequence
            codepoint = (c & 0x1F) << 6;
            if (i + 1 < utf8.size())
            {
                codepoint |= (static_cast<unsigned char>(utf8[i + 1]) & 0x3F);
            }
            i += 2;
        }
        else if ((c & 0xF0) == 0xE0)
        {
            // 3-byte sequence
            codepoint = (c & 0x0F) << 12;
            if (i + 1 < utf8.size())
            {
                codepoint |= (static_cast<unsigned char>(utf8[i + 1]) & 0x3F) << 6;
            }
            if (i + 2 < utf8.size())
            {
                codepoint |= (static_cast<unsigned char>(utf8[i + 2]) & 0x3F);
            }
            i += 3;
        }
        else if ((c & 0xF8) == 0xF0)
        {
            // 4-byte sequence
            codepoint = (c & 0x07) << 18;
            if (i + 1 < utf8.size())
            {
                codepoint |= (static_cast<unsigned char>(utf8[i + 1]) & 0x3F) << 12;
            }
            if (i + 2 < utf8.size())
            {
                codepoint |= (static_cast<unsigned char>(utf8[i + 2]) & 0x3F) << 6;
            }
            if (i + 3 < utf8.size())
            {
                codepoint |= (static_cast<unsigned char>(utf8[i + 3]) & 0x3F);
            }
            i += 4;
        }
        else
        {
            // Invalid UTF-8, skip
            i += 1;
            continue;
        }

        result.push_back(codepoint);
    }

    return result;
}

auto TextRenderer::RenderText(const std::string& text, std::uint32_t color)
    -> std::shared_ptr<WzCanvas>
{
    if (!m_bInitialized || text.empty())
    {
        return nullptr;
    }

    // Convert UTF-8 to UTF-32
    std::u32string utf32 = Utf8ToUtf32(text);
    if (utf32.empty())
    {
        return nullptr;
    }

    FT_GlyphSlot slot = m_pFace->glyph;

    // First pass: calculate total width and max height
    int totalWidth = 0;
    int maxHeight = 0;
    int maxAscender = 0;

    for (char32_t c : utf32)
    {
        FT_UInt glyphIndex = FT_Get_Char_Index(m_pFace, c);
        if (FT_Load_Glyph(m_pFace, glyphIndex, FT_LOAD_DEFAULT))
        {
            continue;
        }

        totalWidth += static_cast<int>(slot->advance.x >> 6);
        maxHeight = std::max(maxHeight, static_cast<int>(slot->bitmap.rows));
        maxAscender = std::max(maxAscender, static_cast<int>(slot->bitmap_top));
    }

    if (totalWidth <= 0 || maxHeight <= 0)
    {
        return nullptr;
    }

    // Use line height based on font metrics
    int lineHeight = static_cast<int>(m_pFace->size->metrics.height >> 6);
    if (lineHeight < maxHeight)
    {
        lineHeight = maxHeight + 2;
    }

    // Create canvas
    auto canvas = std::make_shared<WzCanvas>(totalWidth, lineHeight);

    // Create pixel buffer (RGBA)
    std::vector<std::uint8_t> pixels(static_cast<size_t>(totalWidth * lineHeight * 4), 0);

    // Extract color components
    std::uint8_t colorR = static_cast<std::uint8_t>((color >> 16) & 0xFF);
    std::uint8_t colorG = static_cast<std::uint8_t>((color >> 8) & 0xFF);
    std::uint8_t colorB = static_cast<std::uint8_t>(color & 0xFF);
    std::uint8_t colorA = static_cast<std::uint8_t>((color >> 24) & 0xFF);

    // Second pass: render glyphs
    int penX = 0;
    int baseline = maxAscender;

    for (char32_t c : utf32)
    {
        FT_UInt glyphIndex = FT_Get_Char_Index(m_pFace, c);
        if (FT_Load_Glyph(m_pFace, glyphIndex, FT_LOAD_DEFAULT))
        {
            continue;
        }

        if (FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL))
        {
            continue;
        }

        FT_Bitmap& bitmap = slot->bitmap;
        int glyphX = penX + slot->bitmap_left;
        int glyphY = baseline - slot->bitmap_top;

        // Copy glyph bitmap to canvas
        for (unsigned int row = 0; row < bitmap.rows; ++row)
        {
            for (unsigned int col = 0; col < bitmap.width; ++col)
            {
                int x = glyphX + static_cast<int>(col);
                int y = glyphY + static_cast<int>(row);

                if (x < 0 || x >= totalWidth || y < 0 || y >= lineHeight)
                {
                    continue;
                }

                unsigned char value = bitmap.buffer[row * static_cast<unsigned int>(bitmap.pitch) + col];
                if (value > 0)
                {
                    size_t index = static_cast<size_t>((y * totalWidth + x) * 4);
                    // Blend alpha
                    std::uint8_t alpha = static_cast<std::uint8_t>((value * colorA) / 255);
                    pixels[index] = colorR;
                    pixels[index + 1] = colorG;
                    pixels[index + 2] = colorB;
                    pixels[index + 3] = alpha;
                }
            }
        }

        penX += static_cast<int>(slot->advance.x >> 6);
    }

    canvas->SetPixelData(std::move(pixels));
    return canvas;
}

void TextRenderer::RenderTextDirect(SDL_Renderer* renderer, const std::string& text,
                                     std::int32_t x, std::int32_t y, std::uint32_t color)
{
    if (!m_bInitialized || !renderer || text.empty())
    {
        return;
    }

    auto canvas = RenderText(text, color);
    if (!canvas)
    {
        return;
    }

    // Create SDL texture from canvas
    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STATIC,
        static_cast<int>(canvas->GetWidth()),
        static_cast<int>(canvas->GetHeight())
    );

    if (!texture)
    {
        return;
    }

    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

    const auto& pixelData = canvas->GetPixelData();
    SDL_UpdateTexture(texture, nullptr, pixelData.data(),
                      static_cast<int>(canvas->GetWidth() * 4));

    SDL_FRect destRect = {
        static_cast<float>(x),
        static_cast<float>(y),
        static_cast<float>(canvas->GetWidth()),
        static_cast<float>(canvas->GetHeight())
    };

    SDL_RenderTexture(renderer, texture, nullptr, &destRect);
    SDL_DestroyTexture(texture);
}

} // namespace ms

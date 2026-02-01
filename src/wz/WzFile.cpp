#include "WzFile.h"
#include "WzCanvas.h"
#include "WzCrypto.h"
#include "WzProperty.h"
#include "util/Logger.h"

namespace ms
{

WzFile::WzFile() = default;
WzFile::~WzFile() = default;

WzFile::WzFile(WzFile&&) noexcept = default;
auto WzFile::operator=(WzFile&&) noexcept -> WzFile& = default;

auto WzFile::Open(const std::string& path, const std::uint8_t* iv) -> bool
{
    Close();

    if (!m_reader.Open(path))
    {
        LOG_ERROR("Failed to open WZ file: {}", path);
        return false;
    }

    m_sPath = path;
    m_reader.SetKey(iv);

    if (!ParseHeader())
    {
        LOG_ERROR("Failed to parse WZ header: {}", path);
        Close();
        return false;
    }

    // Create root property
    m_pRoot = std::make_shared<WzProperty>(path);
    m_pRoot->SetWzFile(this);  // Set WzFile for data access

    // Parse directory structure
    if (!ParseDirectories(m_pRoot))
    {
        LOG_ERROR("Failed to parse WZ directories: {}", path);
        Close();
        return false;
    }

    return true;
}

void WzFile::Close()
{
    m_reader.Close();
    m_pRoot.reset();
    m_sPath.clear();
    m_dwStart = 0;
    m_dwHash = 0;
    m_nVersion = 0;
}

auto WzFile::IsOpen() const noexcept -> bool
{
    return m_reader.IsOpen() && m_pRoot != nullptr;
}

auto WzFile::GetRoot() const noexcept -> std::shared_ptr<WzProperty>
{
    return m_pRoot;
}

auto WzFile::FindNode(const std::string& path) -> std::shared_ptr<WzProperty>
{
    if (!m_pRoot)
        return nullptr;

    // Parse path and navigate
    std::shared_ptr<WzProperty> current = m_pRoot;
    std::string::size_type start = 0;
    std::string::size_type end;

    while ((end = path.find('/', start)) != std::string::npos)
    {
        auto segment = path.substr(start, end - start);
        if (!segment.empty())
        {
            current = current->GetChild(segment);
            if (!current)
                return nullptr;
        }
        start = end + 1;
    }

    // Last segment
    if (start < path.size())
    {
        auto segment = path.substr(start);
        if (!segment.empty())
        {
            current = current->GetChild(segment);
        }
    }

    return current;
}

auto WzFile::ParseHeader() -> bool
{
    // Read magic "PKG1"
    auto magic = m_reader.ReadString(4);
    if (magic != u"PKG1")
        return false;

    [[maybe_unused]] auto fileSize = m_reader.Read<std::uint64_t>();
    auto startAt = m_reader.Read<std::uint32_t>();

    // Skip copyright string
    (void)m_reader.ReadString();

    m_reader.SetPosition(startAt);
    auto encryptedVersion = m_reader.Read<std::int16_t>();

    // Try to detect version
    for (int i = 0; i < 0x7FFF; ++i)
    {
        auto fileVersion = static_cast<std::int16_t>(i);
        auto versionHash = GetVersionHash(encryptedVersion, fileVersion);

        if (versionHash != 0)
        {
            m_dwStart = startAt;
            m_dwHash = versionHash;
            m_nVersion = fileVersion;

            auto prevPos = m_reader.GetPosition();

            // Validate by trying to parse
            if (!ParseDirectories(nullptr))
            {
                m_reader.SetPosition(prevPos);
                continue;
            }

            // Reset and return success
            m_reader.SetPosition(prevPos);
            return true;
        }
    }

    return false;
}

auto WzFile::ParseDirectories(std::shared_ptr<WzProperty> parent) -> bool
{
    auto entryCount = m_reader.ReadCompressedInt();

    if (entryCount < 0 || entryCount > 100000)
        return false;

    for (int i = 0; i < entryCount; ++i)
    {
        auto type = m_reader.ReadByte();
        std::u16string name;

        if (type == 1)
        {
            m_reader.Skip(sizeof(std::int32_t) + sizeof(std::uint16_t));
            (void)GetWzOffset();
            continue;
        }
        else if (type == 2)
        {
            auto stringOffset = m_reader.Read<std::int32_t>();
            auto prevPos = m_reader.GetPosition();
            m_reader.SetPosition(m_dwStart + static_cast<std::uint32_t>(stringOffset));
            type = m_reader.ReadByte();
            name = m_reader.ReadWzString();
            m_reader.SetPosition(prevPos);
        }
        else if (type == 3 || type == 4)
        {
            name = m_reader.ReadWzString();
        }
        else
        {
            return false;
        }

        [[maybe_unused]] auto size = m_reader.ReadCompressedInt();
        [[maybe_unused]] auto checksum = m_reader.ReadCompressedInt();
        auto offset = GetWzOffset();

        if (parent == nullptr && offset >= m_reader.GetSize())
            return false;

        if (parent != nullptr)
        {
            // Convert u16string to string for property name
            std::string nameStr(name.begin(), name.end());
            auto child = std::make_shared<WzProperty>(nameStr);
            child->SetWzFile(this);  // Set WzFile for data access

            if (type == 3)
            {
                // Directory - parse children recursively
                auto prevPos = m_reader.GetPosition();
                m_reader.SetPosition(offset);

                if (!ParseDirectories(child))
                {
                    LOG_ERROR("Failed to parse directory: {}", nameStr);
                }

                m_reader.SetPosition(prevPos);
                child->SetNodeType(WzNodeType::Directory);
            }
            else
            {
                // Image (.img file) - set up lazy loading
                child->SetNodeType(WzNodeType::Image);
                child->SetLoadInfo(offset, this);

                // Set up load callback for lazy loading
                auto* wzFile = this;
                child->SetLoadCallback([wzFile](std::shared_ptr<WzProperty> node, std::size_t imgOffset) -> bool {
                    return wzFile->ParseImage(node, imgOffset);
                });
            }

            parent->AddChild(child);
        }
        else
        {
            // Validation mode - check if offset is valid
            if (type == 4)
            {
                auto prevPos = m_reader.GetPosition();
                m_reader.SetPosition(offset);

                if (!m_reader.IsWzImage())
                {
                    m_reader.SetPosition(prevPos);
                    return false;
                }

                m_reader.SetPosition(prevPos);
            }
        }
    }

    return true;
}

auto WzFile::ParseImage(std::shared_ptr<WzProperty> node, std::size_t offset) -> bool
{
    m_reader.SetPosition(offset);

    if (!m_reader.IsWzImage())
        return false;

#ifdef MS_DEBUG_CANVAS
    // Initialize parse path with node name for this image
    m_currentParsePath = node->GetName();
#endif

    return ParsePropertyList(node, offset);
}

auto WzFile::ParsePropertyList(std::shared_ptr<WzProperty> target, std::size_t baseOffset) -> bool
{
    auto entryCount = m_reader.ReadCompressedInt();

    for (std::int32_t i = 0; i < entryCount; ++i)
    {
        auto name = m_reader.ReadStringBlock(baseOffset);
        auto propType = m_reader.Read<std::uint8_t>();

        std::string nameStr(name.begin(), name.end());
        auto prop = std::make_shared<WzProperty>(nameStr);
        prop->SetWzFile(this);  // Set WzFile for data access (sounds, etc.)

#ifdef MS_DEBUG_CANVAS
        // Save current path and append this property name
        std::string savedPath = m_currentParsePath;
        if (!m_currentParsePath.empty())
            m_currentParsePath += "/";
        m_currentParsePath += nameStr;
#endif

        switch (propType)
        {
        case 0: // Null
            break;
        case 0x0B:
        case 2: // UnsignedShort
            prop->SetInt(static_cast<std::int32_t>(m_reader.Read<std::uint16_t>()));
            break;
        case 3: // Int
            prop->SetInt(m_reader.ReadCompressedInt());
            break;
        case 4: // Float
        {
            auto floatType = m_reader.Read<std::uint8_t>();
            if (floatType == 0x80)
                prop->SetFloat(m_reader.Read<float>());
            else
                prop->SetFloat(0.0f);
            break;
        }
        case 5: // Double
            prop->SetDouble(m_reader.Read<double>());
            break;
        case 8: // String
        {
            auto str = m_reader.ReadStringBlock(baseOffset);
            prop->SetString(std::string(str.begin(), str.end()));
            break;
        }
        case 9: // Extended
        {
            auto extOffset = m_reader.Read<std::uint32_t>();
            auto endOfBlock = m_reader.GetPosition() + extOffset;
            ParseExtendedProperty(name, prop, baseOffset);
            if (m_reader.GetPosition() != endOfBlock)
                m_reader.SetPosition(endOfBlock);
            break;
        }
        default:
#ifdef MS_DEBUG_CANVAS
            m_currentParsePath = savedPath;
#endif
            return false;
        }

        target->AddChild(prop);

#ifdef MS_DEBUG_CANVAS
        // Restore path after processing this property
        m_currentParsePath = savedPath;
#endif
    }

    return true;
}

void WzFile::ParseExtendedProperty(const std::u16string& name,
                                   std::shared_ptr<WzProperty> target,
                                   std::size_t baseOffset)
{
    auto propName = m_reader.ReadStringBlock(baseOffset);

    if (propName == u"Property")
    {
        m_reader.Skip(sizeof(std::uint16_t));
        target->SetNodeType(WzNodeType::SubProperty);
        (void)ParsePropertyList(target, baseOffset);
    }
    else if (propName == u"Canvas")
    {
        m_reader.Skip(sizeof(std::uint8_t));
        if (m_reader.Read<std::uint8_t>() == 1)
        {
            m_reader.Skip(sizeof(std::uint16_t));
            (void)ParsePropertyList(target, baseOffset);
        }

        auto canvasData = ParseCanvasProperty();
        auto canvas = LoadCanvasData(canvasData);
        if (canvas)
        {
#ifdef MS_DEBUG_CANVAS
            canvas->SetWzPath(m_currentParsePath);
#endif
            // Read origin from child property if present
            // MapleStory WZ canvases often have an "origin" child specifying the anchor point
            auto originProp = target->GetChild("origin");
            if (originProp)
            {
                auto originVec = originProp->GetVector();
                canvas->SetOrigin(Point2D{originVec.x, originVec.y});
            }

            target->SetCanvas(canvas);
        }
        else
        {
            std::string targetName(name.begin(), name.end());
            LOG_ERROR("WzFile: Failed to load canvas for '{}' (size={}, format={}+{}, {}x{})",
                      targetName, canvasData.size, canvasData.format,
                      static_cast<int>(canvasData.format2), canvasData.width, canvasData.height);
        }
    }
    else if (propName == u"Shape2D#Vector2D")
    {
        auto x = m_reader.ReadCompressedInt();
        auto y = m_reader.ReadCompressedInt();
        target->SetVector(x, y);
    }
    else if (propName == u"Shape2D#Convex2D")
    {
        target->SetNodeType(WzNodeType::Convex2D);
        auto convexCount = m_reader.ReadCompressedInt();
        for (int i = 0; i < convexCount; ++i)
        {
            auto pointProp = std::make_shared<WzProperty>(std::to_string(i));
            pointProp->SetWzFile(this);  // Set WzFile for data access
            ParseExtendedProperty(name, pointProp, baseOffset);
            target->AddChild(pointProp);
        }
    }
    else if (propName == u"Sound_DX8")
    {
        auto sound = ParseSoundProperty();
        target->SetSound(sound);
    }
    else if (propName == u"UOL")
    {
        m_reader.Skip(sizeof(std::uint8_t));
        auto uol = m_reader.ReadStringBlock(baseOffset);
        target->SetString(std::string(uol.begin(), uol.end()));
        target->SetNodeType(WzNodeType::UOL);
    }
}

auto WzFile::ParseCanvasProperty() -> WzCanvasData
{
    WzCanvasData canvas;
    canvas.width = m_reader.ReadCompressedInt();
    canvas.height = m_reader.ReadCompressedInt();
    canvas.format = m_reader.ReadCompressedInt();
    canvas.format2 = m_reader.Read<std::uint8_t>();
    m_reader.Skip(sizeof(std::uint32_t));
    canvas.size = m_reader.Read<std::int32_t>() - 1;
    m_reader.Skip(sizeof(std::uint8_t));

    canvas.offset = m_reader.GetPosition();

    auto header = m_reader.Read<std::uint16_t>();
    if (header != 0x9C78 && header != 0xDA78)
    {
        canvas.isEncrypted = true;
    }

    // Calculate uncompressed size based on format
    switch (canvas.format + canvas.format2)
    {
    case 1: // ARGB4444
        canvas.uncompressedSize = canvas.width * canvas.height * 2;
        break;
    case 2: // ARGB8888
        canvas.uncompressedSize = canvas.width * canvas.height * 4;
        break;
    case 513: // RGB565
        canvas.uncompressedSize = canvas.width * canvas.height * 2;
        break;
    case 517: // DXT3 (MapleStory variant)
        canvas.uncompressedSize = canvas.width * canvas.height / 128;
        break;
    case 1026: // DXT5
    case 2050: // DXT5 (alternate format code)
        // DXT5: 16 bytes per 4x4 block = 1 byte per pixel
        canvas.uncompressedSize = canvas.width * canvas.height;
        break;
    default:
        canvas.uncompressedSize = 0;
        break;
    }

    m_reader.SetPosition(canvas.offset + static_cast<std::size_t>(canvas.size));
    return canvas;
}

auto WzFile::ParseSoundProperty() -> WzSoundData
{
    WzSoundData sound;
    m_reader.Skip(sizeof(std::uint8_t));
    sound.size = m_reader.ReadCompressedInt();
    sound.length = m_reader.ReadCompressedInt();
    // Skip WAVEFORMATEX header (18 bytes) + extra header data
    m_reader.Skip(51);
    sound.frequency = m_reader.Read<std::int32_t>();
    m_reader.Skip(27);

    sound.offset = m_reader.GetPosition();
    m_reader.SetPosition(sound.offset + static_cast<std::size_t>(sound.size));

    return sound;
}

auto WzFile::LoadSoundData(const WzSoundData& soundData) -> std::vector<std::uint8_t>
{
    if (soundData.size <= 0 || soundData.offset == 0)
    {
        return {};
    }

    // Save current position
    auto prevPos = m_reader.GetPosition();

    // Read raw audio data (MP3 format)
    m_reader.SetPosition(soundData.offset);
    auto data = m_reader.ReadBytes(static_cast<std::size_t>(soundData.size));

    // Restore position
    m_reader.SetPosition(prevPos);

    return data;
}

auto WzFile::LoadCanvasData(const WzCanvasData& canvasData) -> std::shared_ptr<WzCanvas>
{
    if (canvasData.width <= 0 || canvasData.height <= 0 || canvasData.size <= 0)
    {
        LOG_ERROR("LoadCanvasData: Invalid dimensions (w={}, h={}, size={})",
                  canvasData.width, canvasData.height, canvasData.size);
        return nullptr;
    }

    // Save current position
    auto prevPos = m_reader.GetPosition();

    // Read compressed data from file
    m_reader.SetPosition(canvasData.offset);
    auto compressedData = m_reader.ReadBytes(static_cast<std::size_t>(canvasData.size));

    // Restore position
    m_reader.SetPosition(prevPos);

    // Decrypt if needed
    if (canvasData.isEncrypted)
    {
        WzCrypto::XorDecrypt(compressedData.data(), compressedData.size());
    }

    // Decompress canvas data using zlib uncompress (same as wzlibcpp)
    auto decompressed = WzCrypto::Decompress(compressedData,
                                              static_cast<std::size_t>(canvasData.uncompressedSize));

    if (decompressed.empty())
    {
        LOG_ERROR("LoadCanvasData: Decompression failed (compressed={}, expected={}, format={}+{})",
                  compressedData.size(), canvasData.uncompressedSize,
                  canvasData.format, static_cast<int>(canvasData.format2));
        return nullptr;
    }

    // Convert pixel format to ARGB8888
    std::vector<std::uint8_t> pixels;
    const auto width = canvasData.width;
    const auto height = canvasData.height;
    const auto format = canvasData.format + canvasData.format2;

    switch (format)
    {
    case 1: // ARGB4444
    {
        pixels.resize(static_cast<std::size_t>(width * height * 4));
        const auto* src = reinterpret_cast<const std::uint16_t*>(decompressed.data());
        auto* dst = pixels.data();

        for (int i = 0; i < width * height; ++i)
        {
            std::uint16_t pixel = src[i];
            // ARGB4444 -> RGBA8888
            dst[i * 4 + 0] = static_cast<std::uint8_t>(((pixel >> 8) & 0x0F) * 17);  // R
            dst[i * 4 + 1] = static_cast<std::uint8_t>(((pixel >> 4) & 0x0F) * 17);  // G
            dst[i * 4 + 2] = static_cast<std::uint8_t>((pixel & 0x0F) * 17);          // B
            dst[i * 4 + 3] = static_cast<std::uint8_t>(((pixel >> 12) & 0x0F) * 17); // A
        }
        break;
    }
    case 2: // ARGB8888
    {
        // Already in correct format, just reorder to RGBA
        pixels.resize(static_cast<std::size_t>(width * height * 4));
        const auto* src = decompressed.data();
        auto* dst = pixels.data();

        for (int i = 0; i < width * height; ++i)
        {
            // ARGB -> RGBA
            dst[i * 4 + 0] = src[i * 4 + 1]; // R
            dst[i * 4 + 1] = src[i * 4 + 2]; // G
            dst[i * 4 + 2] = src[i * 4 + 3]; // B
            dst[i * 4 + 3] = src[i * 4 + 0]; // A
        }
        break;
    }
    case 513: // RGB565
    {
        pixels.resize(static_cast<std::size_t>(width * height * 4));
        const auto* src = reinterpret_cast<const std::uint16_t*>(decompressed.data());
        auto* dst = pixels.data();

        for (int i = 0; i < width * height; ++i)
        {
            std::uint16_t pixel = src[i];
            // RGB565 -> RGBA8888
            dst[i * 4 + 0] = static_cast<std::uint8_t>(((pixel >> 11) & 0x1F) * 255 / 31); // R
            dst[i * 4 + 1] = static_cast<std::uint8_t>(((pixel >> 5) & 0x3F) * 255 / 63);  // G
            dst[i * 4 + 2] = static_cast<std::uint8_t>((pixel & 0x1F) * 255 / 31);          // B
            dst[i * 4 + 3] = 255;                                                           // A
        }
        break;
    }
    case 517: // DXT3
    {
        pixels = DecompressDxt3(decompressed, width, height);
        break;
    }
    case 1026: // DXT5
    case 2050: // DXT5 (alternate format code)
    {
        pixels = DecompressDxt5(decompressed, width, height);
        break;
    }
    default:
        LOG_ERROR("Unknown canvas format: {}", format);
        return nullptr;
    }

    if (pixels.empty())
        return nullptr;

    // Create canvas object
    auto canvas = std::make_shared<WzCanvas>(width, height);
    canvas->SetPixelData(std::move(pixels));

    return canvas;
}

auto WzFile::DecompressDxt3(const std::vector<std::uint8_t>& data,
                             std::int32_t width, std::int32_t height) -> std::vector<std::uint8_t>
{
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width * height * 4));

    const int blocksX = (width + 3) / 4;
    const int blocksY = (height + 3) / 4;
    const auto* src = data.data();

    for (int by = 0; by < blocksY; ++by)
    {
        for (int bx = 0; bx < blocksX; ++bx)
        {
            // Read alpha block (8 bytes, 4-bit alpha per pixel)
            std::uint8_t alphaBlock[16];
            for (int i = 0; i < 8; ++i)
            {
                std::uint8_t alphaByte = *src++;
                alphaBlock[i * 2] = static_cast<std::uint8_t>((alphaByte & 0x0F) * 17);
                alphaBlock[i * 2 + 1] = static_cast<std::uint8_t>((alphaByte >> 4) * 17);
            }

            // Read color block (8 bytes)
            std::uint16_t c0 = static_cast<std::uint16_t>(src[0] | (src[1] << 8));
            std::uint16_t c1 = static_cast<std::uint16_t>(src[2] | (src[3] << 8));
            src += 4;

            // Decode colors
            std::uint8_t colors[4][4]; // RGBA
            // Color 0
            colors[0][0] = static_cast<std::uint8_t>(((c0 >> 11) & 0x1F) * 255 / 31);
            colors[0][1] = static_cast<std::uint8_t>(((c0 >> 5) & 0x3F) * 255 / 63);
            colors[0][2] = static_cast<std::uint8_t>((c0 & 0x1F) * 255 / 31);
            colors[0][3] = 255;
            // Color 1
            colors[1][0] = static_cast<std::uint8_t>(((c1 >> 11) & 0x1F) * 255 / 31);
            colors[1][1] = static_cast<std::uint8_t>(((c1 >> 5) & 0x3F) * 255 / 63);
            colors[1][2] = static_cast<std::uint8_t>((c1 & 0x1F) * 255 / 31);
            colors[1][3] = 255;
            // Color 2 (2/3 c0 + 1/3 c1)
            colors[2][0] = static_cast<std::uint8_t>((2 * colors[0][0] + colors[1][0]) / 3);
            colors[2][1] = static_cast<std::uint8_t>((2 * colors[0][1] + colors[1][1]) / 3);
            colors[2][2] = static_cast<std::uint8_t>((2 * colors[0][2] + colors[1][2]) / 3);
            colors[2][3] = 255;
            // Color 3 (1/3 c0 + 2/3 c1)
            colors[3][0] = static_cast<std::uint8_t>((colors[0][0] + 2 * colors[1][0]) / 3);
            colors[3][1] = static_cast<std::uint8_t>((colors[0][1] + 2 * colors[1][1]) / 3);
            colors[3][2] = static_cast<std::uint8_t>((colors[0][2] + 2 * colors[1][2]) / 3);
            colors[3][3] = 255;

            // Read color indices
            std::uint32_t indices = static_cast<std::uint32_t>(src[0] | (src[1] << 8) |
                                                                (src[2] << 16) | (src[3] << 24));
            src += 4;

            // Write pixels
            for (int py = 0; py < 4; ++py)
            {
                for (int px = 0; px < 4; ++px)
                {
                    int x = bx * 4 + px;
                    int y = by * 4 + py;
                    if (x >= width || y >= height)
                        continue;

                    int idx = (indices >> ((py * 4 + px) * 2)) & 0x03;
                    auto* dst = &pixels[static_cast<std::size_t>((y * width + x) * 4)];
                    dst[0] = colors[idx][0];
                    dst[1] = colors[idx][1];
                    dst[2] = colors[idx][2];
                    dst[3] = alphaBlock[py * 4 + px];
                }
            }
        }
    }

    return pixels;
}

auto WzFile::DecompressDxt5(const std::vector<std::uint8_t>& data,
                             std::int32_t width, std::int32_t height) -> std::vector<std::uint8_t>
{
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width * height * 4));

    const int blocksX = (width + 3) / 4;
    const int blocksY = (height + 3) / 4;
    const auto* src = data.data();

    for (int by = 0; by < blocksY; ++by)
    {
        for (int bx = 0; bx < blocksX; ++bx)
        {
            // Read alpha block (8 bytes)
            std::uint8_t a0 = *src++;
            std::uint8_t a1 = *src++;

            // Build alpha lookup table
            std::uint8_t alphaLookup[8];
            alphaLookup[0] = a0;
            alphaLookup[1] = a1;

            if (a0 > a1)
            {
                alphaLookup[2] = static_cast<std::uint8_t>((6 * a0 + 1 * a1) / 7);
                alphaLookup[3] = static_cast<std::uint8_t>((5 * a0 + 2 * a1) / 7);
                alphaLookup[4] = static_cast<std::uint8_t>((4 * a0 + 3 * a1) / 7);
                alphaLookup[5] = static_cast<std::uint8_t>((3 * a0 + 4 * a1) / 7);
                alphaLookup[6] = static_cast<std::uint8_t>((2 * a0 + 5 * a1) / 7);
                alphaLookup[7] = static_cast<std::uint8_t>((1 * a0 + 6 * a1) / 7);
            }
            else
            {
                alphaLookup[2] = static_cast<std::uint8_t>((4 * a0 + 1 * a1) / 5);
                alphaLookup[3] = static_cast<std::uint8_t>((3 * a0 + 2 * a1) / 5);
                alphaLookup[4] = static_cast<std::uint8_t>((2 * a0 + 3 * a1) / 5);
                alphaLookup[5] = static_cast<std::uint8_t>((1 * a0 + 4 * a1) / 5);
                alphaLookup[6] = 0;
                alphaLookup[7] = 255;
            }

            // Read 48 bits of alpha indices (6 bytes)
            std::uint64_t alphaIndices = 0;
            for (int i = 0; i < 6; ++i)
            {
                alphaIndices |= static_cast<std::uint64_t>(*src++) << (i * 8);
            }

            // Read color block (same as DXT3)
            std::uint16_t c0 = static_cast<std::uint16_t>(src[0] | (src[1] << 8));
            std::uint16_t c1 = static_cast<std::uint16_t>(src[2] | (src[3] << 8));
            src += 4;

            // Decode colors
            std::uint8_t colors[4][4];
            colors[0][0] = static_cast<std::uint8_t>(((c0 >> 11) & 0x1F) * 255 / 31);
            colors[0][1] = static_cast<std::uint8_t>(((c0 >> 5) & 0x3F) * 255 / 63);
            colors[0][2] = static_cast<std::uint8_t>((c0 & 0x1F) * 255 / 31);
            colors[0][3] = 255;

            colors[1][0] = static_cast<std::uint8_t>(((c1 >> 11) & 0x1F) * 255 / 31);
            colors[1][1] = static_cast<std::uint8_t>(((c1 >> 5) & 0x3F) * 255 / 63);
            colors[1][2] = static_cast<std::uint8_t>((c1 & 0x1F) * 255 / 31);
            colors[1][3] = 255;

            colors[2][0] = static_cast<std::uint8_t>((2 * colors[0][0] + colors[1][0]) / 3);
            colors[2][1] = static_cast<std::uint8_t>((2 * colors[0][1] + colors[1][1]) / 3);
            colors[2][2] = static_cast<std::uint8_t>((2 * colors[0][2] + colors[1][2]) / 3);
            colors[2][3] = 255;

            colors[3][0] = static_cast<std::uint8_t>((colors[0][0] + 2 * colors[1][0]) / 3);
            colors[3][1] = static_cast<std::uint8_t>((colors[0][1] + 2 * colors[1][1]) / 3);
            colors[3][2] = static_cast<std::uint8_t>((colors[0][2] + 2 * colors[1][2]) / 3);
            colors[3][3] = 255;

            std::uint32_t indices = static_cast<std::uint32_t>(src[0] | (src[1] << 8) |
                                                                (src[2] << 16) | (src[3] << 24));
            src += 4;

            // Write pixels
            for (int py = 0; py < 4; ++py)
            {
                for (int px = 0; px < 4; ++px)
                {
                    int x = bx * 4 + px;
                    int y = by * 4 + py;
                    if (x >= width || y >= height)
                        continue;

                    int colorIdx = (indices >> ((py * 4 + px) * 2)) & 0x03;
                    int alphaIdx = (alphaIndices >> ((py * 4 + px) * 3)) & 0x07;

                    auto* dst = &pixels[static_cast<std::size_t>((y * width + x) * 4)];
                    dst[0] = colors[colorIdx][0];
                    dst[1] = colors[colorIdx][1];
                    dst[2] = colors[colorIdx][2];
                    dst[3] = alphaLookup[alphaIdx];
                }
            }
        }
    }

    return pixels;
}

auto WzFile::GetWzOffset() -> std::uint32_t
{
    auto offset = static_cast<std::uint32_t>(m_reader.GetPosition());
    offset = ~(offset - m_dwStart);
    offset *= m_dwHash;
    offset -= WzKeys::OffsetKey;
    offset = (offset << (offset & 0x1Fu)) | (offset >> (32 - (offset & 0x1Fu)));
    auto encryptedOffset = m_reader.Read<std::uint32_t>();
    offset ^= encryptedOffset;
    offset += m_dwStart * 2;
    return offset;
}

auto WzFile::GetVersionHash(std::int32_t encrypted, std::int32_t real) -> std::uint32_t
{
    std::int32_t versionHash = 0;
    auto versionStr = std::to_string(real);

    for (char c : versionStr)
    {
        versionHash = (versionHash * 32) + static_cast<std::int32_t>(c) + 1;
    }

    auto a = (versionHash >> 24) & 0xFF;
    auto b = (versionHash >> 16) & 0xFF;
    auto c = (versionHash >> 8) & 0xFF;
    auto d = versionHash & 0xFF;

    auto decrypted = static_cast<std::uint8_t>(0xFF ^ a ^ b ^ c ^ d);

    if (decrypted == encrypted)
        return static_cast<std::uint32_t>(versionHash);

    return 0;
}

} // namespace ms

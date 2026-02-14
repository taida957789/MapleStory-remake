#include "WzFile.h"
#include "WzCanvas.h"
#include "WzCrypto.h"
#include "WzDirectory.h"
#include "WzImage.h"
#include "WzProperty.h"
#include "WzRaw.h"
#include "WzVideo.h"

namespace ms
{

WzFile::WzFile() = default;
WzFile::~WzFile() = default;

WzFile::WzFile(WzFile&&) noexcept = default;
auto WzFile::operator=(WzFile&&) noexcept -> WzFile& = default;

// IWzSource interface Open() - delegates to Open(path, iv)
auto WzFile::Open(const std::string& path) -> bool
{
    // Default to ZERO_IV for auto-detection
    return Open(path, WzKeys::ZERO_IV);
}

// Legacy Open() with explicit IV parameter
auto WzFile::Open(const std::string& path, const std::uint8_t* iv) -> bool
{
    Close();

    if (!m_reader.Open(path))
    {
        // Failed to open WZ file
        return false;
    }

    m_sPath = path;
    m_reader.SetKey(iv);

    if (!ParseHeader())
    {
        // Failed to parse WZ header
        Close();
        return false;
    }

    // Create root directory
    m_pRoot = std::make_shared<WzDirectory>(path);

    // Try normal parsing first
    if (ParseDirectories(m_pRoot))
    {
        return true;
    }

    // If failed, try 64-bit version detection
    if (Try64BitVersionDetection())
    {
        return true;
    }

    // Failed to parse WZ directories
    Close();
    return false;
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

auto WzFile::IsOpen() const -> bool
{
    return m_reader.IsOpen() && m_pRoot != nullptr;
}

auto WzFile::GetRoot() const -> std::shared_ptr<WzDirectory>
{
    return m_pRoot;
}

auto WzFile::FindNode(const std::string& path) -> std::shared_ptr<WzNode>
{
    if (!m_pRoot)
        return nullptr;

    // Parse path and navigate
    std::shared_ptr<WzNode> current = m_pRoot;
    std::string::size_type start = 0;
    std::string::size_type end;

    while ((end = path.find('/', start)) != std::string::npos)
    {
        auto segment = path.substr(start, end - start);
        if (!segment.empty())
        {
            // Check node type and navigate accordingly
            if (auto dir = std::dynamic_pointer_cast<WzDirectory>(current))
            {
                current = dir->GetChild(segment);
            }
            else if (auto img = std::dynamic_pointer_cast<WzImage>(current))
            {
                // Trigger lazy loading if needed
                if (!img->IsLoaded())
                {
                    if (!LoadImage(img.get()))
                        return nullptr;
                }
                current = img->GetProperty(segment);
            }
            else if (auto prop = std::dynamic_pointer_cast<WzProperty>(current))
            {
                current = prop->GetChild(segment);
            }
            else
            {
                return nullptr;
            }

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
            if (auto dir = std::dynamic_pointer_cast<WzDirectory>(current))
            {
                current = dir->GetChild(segment);
            }
            else if (auto img = std::dynamic_pointer_cast<WzImage>(current))
            {
                // Trigger lazy loading if needed
                if (!img->IsLoaded())
                {
                    if (!LoadImage(img.get()))
                        return nullptr;
                }
                current = img->GetProperty(segment);
            }
            else if (auto prop = std::dynamic_pointer_cast<WzProperty>(current))
            {
                current = prop->GetChild(segment);
            }
        }
    }

    return current;
}

auto WzFile::LoadImage(WzImage* image) -> bool
{
    if (!image)
        return false;

    // Check if already loaded
    if (image->IsLoaded())
        return true;

    // Parse the image at its stored offset
    if (!ParseImage(image))
        return false;

    // Mark as loaded
    image->MarkLoaded();
    return true;
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

auto WzFile::ParseDirectories(std::shared_ptr<WzDirectory> parent) -> bool
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

        auto size = m_reader.ReadCompressedInt();
        auto checksum = m_reader.ReadCompressedInt();
        auto offset = GetWzOffset();

        if (parent == nullptr && offset >= m_reader.GetSize())
            return false;

        if (parent != nullptr)
        {
            // Convert u16string to string for node name
            std::string nameStr(name.begin(), name.end());

            if (type == 3)
            {
                // Directory - create WzDirectory node
                auto dir = std::make_shared<WzDirectory>(nameStr);

                // Parse children recursively
                auto prevPos = m_reader.GetPosition();
                m_reader.SetPosition(offset);

                if (!ParseDirectories(dir))
                {
                    // Failed to parse directory
                }

                m_reader.SetPosition(prevPos);

                // Add to parent
                parent->AddChild(dir);
            }
            else
            {
                // Image (.img file) - create WzImage node
                auto img = std::make_shared<WzImage>(nameStr);
                img->SetOffset(offset);
                img->SetSize(static_cast<std::size_t>(size));
                img->SetChecksum(static_cast<std::uint32_t>(checksum));

                // Set weak_ptr to this WzFile for lazy loading
                // We need to use shared_from_this, but it may not be available during construction
                // Store the file pointer for now, it will be set properly after Open() completes
                img->SetWzFile(shared_from_this());

                // Add to parent
                parent->AddChild(img);
            }
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

auto WzFile::ParseImage(WzImage* image) -> bool
{
    if (!image)
        return false;

    m_reader.SetPosition(image->GetOffset());

    if (!m_reader.IsWzImage())
        return false;

#ifdef MS_DEBUG_CANVAS
    // Initialize parse path with image name
    m_currentParsePath = image->GetName();
#endif

    return ParsePropertyList(image, image->GetOffset());
}

auto WzFile::ParsePropertyList(WzImage* target, std::size_t baseOffset) -> bool
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

        target->AddProperty(prop);

#ifdef MS_DEBUG_CANVAS
        // Restore path after processing this property
        m_currentParsePath = savedPath;
#endif
    }

    return true;
}

auto WzFile::ParsePropertyChildren(std::shared_ptr<WzProperty> target, std::size_t baseOffset) -> bool
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
        (void)ParsePropertyChildren(target, baseOffset);
    }
    else if (propName == u"Canvas")
    {
        m_reader.Skip(sizeof(std::uint8_t));
        if (m_reader.Read<std::uint8_t>() == 1)
        {
            m_reader.Skip(sizeof(std::uint16_t));
            (void)ParsePropertyChildren(target, baseOffset);
        }

        auto canvasData = ParseCanvasProperty();
        auto canvas = LoadCanvasData(canvasData);
        if (canvas)
        {
#ifdef MS_DEBUG_CANVAS
            canvas->SetWzPath(m_currentParsePath);
#endif
            target->SetCanvas(canvas);
        }
        else
        {
            std::string targetName(name.begin(), name.end());
            // Failed to load canvas
        }
    }
    else if (propName == u"Canvas#Video")
    {
        m_reader.Skip(sizeof(std::uint8_t));
        if (m_reader.Read<std::uint8_t>() == 1)
        {
            m_reader.Skip(sizeof(std::uint16_t));
            (void)ParsePropertyChildren(target, baseOffset);
        }
        auto videoData = ParseVideoProperty();
        auto video = LoadVideoData(videoData);
        if (video)
        {
            target->SetVideo(video);
        }
    }
    else if (propName == u"RawData")
    {
        auto type = m_reader.Read<std::uint8_t>();
        if (m_reader.Read<std::uint8_t>() == 1)
        {
            m_reader.Skip(sizeof(std::uint16_t));
            (void)ParsePropertyChildren(target, baseOffset);
        }
        auto rawData = ParseRawDataProperty(type);
        auto raw = LoadRawDataData(rawData);
        if (raw)
        {
            target->SetRaw(raw);
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

auto WzFile::ParseVideoProperty() -> WzVideoData
{
    WzVideoData video;
    video.type = m_reader.Read<std::uint8_t>();
    video.size = static_cast<std::size_t>(m_reader.ReadCompressedInt());
    video.offset = m_reader.GetPosition();
    m_reader.SetPosition(video.offset + video.size);
    return video;
}

auto WzFile::ParseRawDataProperty(int type) -> WzRawData
{
    WzRawData rawData;
    rawData.type = type;
    rawData.size = static_cast<std::size_t>(m_reader.ReadCompressedInt());
    rawData.offset = m_reader.GetPosition();
    m_reader.SetPosition(rawData.offset + rawData.size);
    return rawData;
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

auto WzFile::LoadRawDataData(const WzRawData& rawData) -> std::shared_ptr<WzRaw>
{
    if (rawData.size == 0 || rawData.offset == 0)
        return nullptr;

    auto prevPos = m_reader.GetPosition();
    m_reader.SetPosition(rawData.offset);
    auto data = m_reader.ReadBytes(rawData.size);
    m_reader.SetPosition(prevPos);

    auto raw = std::make_shared<WzRaw>();
    raw->SetType(rawData.type);
    raw->SetData(data);
    return raw;
}

auto WzFile::LoadVideoData(const WzVideoData& videoData) -> std::shared_ptr<WzVideo>
{
    if (videoData.size == 0 || videoData.offset == 0)
        return nullptr;

    auto prevPos = m_reader.GetPosition();
    m_reader.SetPosition(videoData.offset);
    auto data = m_reader.ReadBytes(videoData.size);
    m_reader.SetPosition(prevPos);

    auto video = std::make_shared<WzVideo>();
    video->SetType(videoData.type);
    video->SetData(data);
    return video;
}

auto WzFile::LoadCanvasData(const WzCanvasData& canvasData) -> std::shared_ptr<WzCanvas>
{
    if (canvasData.width <= 0 || canvasData.height <= 0 || canvasData.size <= 0)
    {
        // Invalid canvas dimensions
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
        // Decompression failed
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
        // WZ stores ARGB8888 as 0xAARRGGBB (DirectX format)
        // On little-endian, bytes in memory are: B, G, R, A (BGRA byte order)
        // Convert to RGBA byte order for SDL
        pixels.resize(static_cast<std::size_t>(width * height * 4));
        const auto* src = decompressed.data();
        auto* dst = pixels.data();

        for (int i = 0; i < width * height; ++i)
        {
            // BGRA (memory layout) -> RGBA
            dst[i * 4 + 0] = src[i * 4 + 2]; // R (from byte 2)
            dst[i * 4 + 1] = src[i * 4 + 1]; // G (from byte 1)
            dst[i * 4 + 2] = src[i * 4 + 0]; // B (from byte 0)
            dst[i * 4 + 3] = src[i * 4 + 3]; // A (from byte 3)
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
        // Unknown canvas format
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

auto WzFile::Try64BitVersionDetection() -> bool
{
    // 64-bit WZ files use version range 770-780
    // Reference: MapleLib wzVersionHeader64bit_start = 770
    for (int version = 770; version <= 780; ++version)
    {
        if (TryDecodeWithVersion(version))
        {
            m_nVersion = static_cast<std::int16_t>(version);
            return true;
        }
    }
    return false;
}

auto WzFile::TryDecodeWithVersion(int version) -> bool
{
    // Save current position
    auto currentPos = m_reader.GetPosition();

    // Reset to start of directory parsing
    m_reader.SetPosition(m_dwStart);

    // Try to parse with this version
    m_pRoot = std::make_shared<WzDirectory>(m_sPath);

    // Temporarily set version for parsing attempt
    m_nVersion = static_cast<std::int16_t>(version);

    // Calculate version hash for this version
    // For 64-bit versions, we need to compute the hash differently
    // We use a dummy encrypted value of 0 to generate hash from version
    m_dwHash = GetVersionHash(0, version);
    if (m_dwHash == 0)
    {
        // If hash calculation fails, use version number directly as hash
        // This is common for 64-bit versions
        std::int32_t versionHash = 0;
        auto versionStr = std::to_string(version);
        for (char c : versionStr)
        {
            versionHash = (versionHash * 32) + static_cast<std::int32_t>(c) + 1;
        }
        m_dwHash = static_cast<std::uint32_t>(versionHash);
    }

    bool success = ParseDirectories(m_pRoot);

    if (!success)
    {
        // Restore position and clear root on failure
        m_reader.SetPosition(currentPos);
        m_pRoot.reset();
    }

    return success;
}

} // namespace ms

#pragma once

#include "BufferedStream.h"

#include <spine/spine.h>

#include <string>
#include <vector>

namespace Spine
{

// Reconstructed from MapleStory binary.
// C++ skeleton binary reader that wraps spine-c's attachment/skeleton types.
// Layout: { float Scale; spAttachmentLoader* attachmentLoader; } (8 bytes)
class SkeletonBinary
{
public:
    float Scale                          = 1.0f;
    spAttachmentLoader* attachmentLoader = nullptr;

    // Main entry point — parses a complete skeleton from a BufferedStream
    auto Read(spSkeletonJson* self, BufferedStream& input) -> spSkeletonData*;

    // --- Primitive readers ---
    auto ReadSByte(BufferedStream& input) -> char;
    auto ReadBoolean(BufferedStream& input) -> bool;
    auto ReadFloat(BufferedStream& input) -> float;
    auto ReadInt(BufferedStream& input) -> int;
    auto ReadInt(BufferedStream& input, bool optimizePositive) -> int;
    auto ReadString(BufferedStream& input) -> std::string;
    void ReadCurve(BufferedStream& input, int frameIndex, spCurveTimeline* timeline);

    // --- Array readers ---
    auto ReadFloatArray(BufferedStream& input, float scale) -> std::vector<float>;
    auto ReadShortArray(BufferedStream& input) -> std::vector<int>;
    auto ReadIntArray(BufferedStream& input) -> std::vector<int>;

    // --- Structure readers ---
    auto ReadAttachment(BufferedStream& input, spSkin* skin,
                        const std::string& name, bool nonessential) -> spAttachment*;
    auto ReadSkin(BufferedStream& input, const std::string& name,
                  bool nonessential) -> spSkin*;
    void ReadAnimation(const std::string& name, BufferedStream& input,
                       spSkeletonData* skeletonData, int nonessential);

private:
    void ReadUtf8Slow(BufferedStream& input, std::string& chars,
                      unsigned int charCount, int b);
};

} // namespace Spine

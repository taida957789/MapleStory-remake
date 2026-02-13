#pragma once

namespace ms
{

/**
 * @brief Interface for game objects in the update loop
 *
 * Based on IGObj from the original MapleStory client (v1029).
 * Objects registered in CWvsApp's game object pool implement this
 * interface to receive per-frame Update() calls.
 *
 * VFT layout (sizeof=0x4):
 *   0x00  void Update()
 */
class IGObj
{
public:
    virtual ~IGObj() = default;

    virtual void Update() = 0;
};

} // namespace ms

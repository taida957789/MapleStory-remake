#pragma once

#include <cstdint>

namespace ms
{

/// Check if job ID belongs to the Kinesis class tree.
[[nodiscard]] bool is_kinesis_job(std::int32_t nJob);

} // namespace ms

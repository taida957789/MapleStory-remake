#include "JobConstants.h"

namespace ms
{

bool is_kinesis_job(std::int32_t nJob)
{
    return nJob == 14000 || nJob == 14200 || nJob == 14210
        || nJob == 14211 || nJob == 14212;
}

} // namespace ms

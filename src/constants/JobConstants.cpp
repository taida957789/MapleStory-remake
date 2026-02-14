#include "JobConstants.h"

namespace ms
{

// --- Explorer Warrior ---

bool is_hero_job(std::int32_t nJob)
{
    return nJob == 110 || nJob == 111 || nJob == 112;
}

bool is_paladin_job(std::int32_t nJob)
{
    return nJob == 120 || nJob == 121 || nJob == 122;
}

bool is_darkknight_job(std::int32_t nJob)
{
    return nJob == 130 || nJob == 131 || nJob == 132;
}

// --- Explorer Mage ---

bool is_mage1_job(std::int32_t nJob)
{
    return nJob == 210 || nJob == 211 || nJob == 212;
}

bool is_mage2_job(std::int32_t nJob)
{
    return nJob == 220 || nJob == 221 || nJob == 222;
}

bool is_mage3_job(std::int32_t nJob)
{
    return nJob == 230 || nJob == 231 || nJob == 232;
}

// --- Explorer Bowman ---

bool is_bowmaster_job(std::int32_t nJob)
{
    return nJob == 310 || nJob == 311 || nJob == 312;
}

bool is_crossbow_job(std::int32_t nJob)
{
    return nJob == 320 || nJob == 321 || nJob == 322;
}

// --- Explorer Thief ---

bool is_nightlord_job(std::int32_t nJob)
{
    return nJob == 410 || nJob == 411 || nJob == 412;
}

bool is_shadower_job(std::int32_t nJob)
{
    return nJob == 420 || nJob == 421 || nJob == 422;
}

bool is_dual_job_born(std::int32_t nJob, std::int16_t nSubJob)
{
    return nJob / 1000 == 0 && nSubJob == 1;
}

// --- Explorer Pirate ---

bool is_viper_job(std::int32_t nJob)
{
    return nJob == 510 || nJob == 511 || nJob == 512;
}

bool is_captain_job(std::int32_t nJob)
{
    return nJob == 520 || nJob == 521 || nJob == 522;
}

bool is_cannonshooter_job(std::int32_t nJob)
{
    return nJob / 10 == 53 || nJob == 501;
}

// --- Cygnus Knights ---

bool is_cygnus_job(std::int32_t nJob)
{
    return nJob / 1000 == 1;
}

// --- Mihile ---

bool is_michael_job(std::int32_t nJob)
{
    return nJob / 100 == 51 || nJob == 5000;
}

// --- Heroes of Maple ---

bool is_aran_job(std::int32_t nJob)
{
    return nJob / 100 == 21 || nJob == 2000;
}

bool is_evan_job(std::int32_t nJob)
{
    return nJob / 100 == 22 || nJob == 2001;
}

bool is_phantom_job(std::int32_t nJob)
{
    return nJob / 100 == 24 || nJob == 2003;
}

bool is_luminous_job(std::int32_t nJob)
{
    return nJob / 100 == 27 || nJob == 2004;
}

// --- Resistance ---

bool is_dslayer_job_born(std::int32_t nJob)
{
    return nJob / 100 == 31 || nJob == 3001;
}

bool is_bmage_job(std::int32_t nJob)
{
    return nJob / 100 == 32;
}

bool is_wildhunter_job(std::int32_t nJob)
{
    return nJob / 100 == 33;
}

bool is_mechanic_job(std::int32_t nJob)
{
    return nJob / 100 == 35;
}

bool is_res_hybrid_job(std::int32_t nJob)
{
    return nJob / 100 == 36 || nJob == 3002;
}

// --- Nova ---

bool is_kaiser_job(std::int32_t nJob)
{
    return nJob / 100 == 61 || nJob == 6000;
}

bool is_angelic_burster_job(std::int32_t nJob)
{
    return nJob / 100 == 65 || nJob == 6001;
}

// --- Zero ---

bool is_zero_job(std::int32_t nJob)
{
    return nJob == 10000 || nJob == 10100 || nJob == 10110
        || nJob == 10111 || nJob == 10112;
}

// --- Kinesis ---

bool is_kinesis_job(std::int32_t nJob)
{
    return nJob == 14000 || nJob == 14200 || nJob == 14210
        || nJob == 14211 || nJob == 14212;
}

// --- Beginner ---

bool is_beginner_job(std::int32_t nJob)
{
    if (nJob > 6001)
    {
        if (nJob == 13000 || nJob == 14000)
            return true;
        return false;
    }
    if (nJob >= 6000)
        return true;
    if (nJob > 3002)
    {
        if (nJob == 5000)
            return true;
        return false;
    }
    if (nJob >= 3001)
        return true;
    if (nJob >= 2001 && nJob <= 2005)
        return true;
    if (nJob % 1000 == 0)
        return true;
    return nJob / 100 == 8000;
}

} // namespace ms

#pragma once

#include <cstdint>

#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace mkpoker::util
{
#ifdef _MSC_VER

    inline uint8_t cross_popcnt16(const uint16_t mask) { return static_cast<uint8_t>(__popcnt16(mask)); }

    inline uint8_t cross_popcnt64(const uint64_t mask) { return static_cast<uint8_t>(__popcnt64(mask)); }

    inline uint8_t cross_tzcnt16(const uint16_t mask)
    {
        unsigned long index;
        _BitScanForward(&index, mask);
        return static_cast<uint8_t>(index);
    }

    inline uint8_t cross_tzcnt64(const uint64_t mask)
    {
        unsigned long index;
        _BitScanForward64(&index, mask);
        return static_cast<uint8_t>(index);
    }

    inline uint8_t cross_lzcnt16(const uint16_t mask) { return static_cast<uint8_t>(__lzcnt16(mask)); }

    inline uint8_t cross_lzcnt64(const uint64_t mask) { return static_cast<uint8_t>(__lzcnt64(mask)); }

#elif defined(__clang__) || defined(__GNUC__)

    inline uint8_t cross_popcnt16(const uint16_t mask) { return __builtin_popcount(mask); }

    inline uint8_t cross_popcnt64(const uint64_t mask) { return __builtin_popcountll(mask); }

    inline uint8_t cross_tzcnt16(const uint16_t mask)
    {
        if (mask == 0)
            return 0;
        return __builtin_ctz(mask);
    }

    inline uint8_t cross_tzcnt64(const uint64_t mask)
    {
        if (mask == 0)
            return 0;
        return __builtin_ctzll(mask);
    }

    inline uint8_t cross_lzcnt16(const uint16_t mask)
    {
        if (mask == 0)
            return 0;
        return __builtin_clz(mask) - 16;
    }

    inline uint8_t cross_lzcnt64(const uint64_t mask)
    {
        if (mask == 0)
            return 0;
        return __builtin_clzll(mask);
    }

#else

    static_assert(false, "cross-platform bit operations are implemented for gcc, clang and msvc only");

#endif

    // wrappers

    inline uint8_t cross_idx_low16(const uint16_t mask) { return cross_tzcnt16(mask); }
    inline uint8_t cross_idx_low64(const uint64_t mask) { return cross_tzcnt64(mask); }
    inline uint8_t cross_idx_high16(const uint16_t mask)
    {
        if (mask == 0)
            return 0;
        return static_cast<uint8_t>(15 - cross_lzcnt16(mask));
    }
    inline uint8_t cross_idx_high64(const uint64_t mask)
    {
        if (mask == 0)
            return 0;
        return static_cast<uint8_t>(63 - cross_lzcnt64(mask));
    }

}    // namespace mkpoker::util

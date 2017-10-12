#pragma once

#include <cstdint>
#include <string>

#ifdef _WIN32
#pragma warning(disable : 4307)
#endif

template <std::uint32_t FnvPrime, std::uint32_t OffsetBasis>
struct basic_fnv_1_32
{
    constexpr static uint32_t offsetBasis = OffsetBasis;
    constexpr static uint32_t prime = FnvPrime;

    std::uint32_t operator()(std::string const& text) const
    {
        std::uint32_t hash = OffsetBasis;
        for (std::string::const_iterator it = text.begin(), end = text.end();
            it != end; ++it) {
            hash *= FnvPrime;
            hash ^= *it;
        }

        return hash;
    }

    constexpr static inline uint32_t hash(const char* const aString,
        const size_t length,
        const uint32_t value = OffsetBasis)
    {
        return (length == 0) ? value
            : hash(aString + 1, length - 1,
            (value * FnvPrime) ^ uint32_t(aString[0]));
    };
};

using fnv_1_32 = basic_fnv_1_32<16777619, 2166136261>;

inline constexpr uint32_t operator"" _fnv1_32(const char* aString,
    const size_t aStrlen)
{
    using hash_type = fnv_1_32;
    return hash_type::hash(aString, aStrlen, hash_type::offsetBasis);
}

// MSVC only does it at compile time when used with a template,
// for some to me unknown reason
template <uint32_t meow>
struct FNV1_TemplateTrick
{
    static constexpr uint32_t hash = meow;
};

#ifndef FNV1_HASH
#define FNV1_HASH(x) FNV1_TemplateTrick<x##_fnv1_32>::hash
#endif

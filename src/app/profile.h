#ifndef JCMR_APP_PROFILE_H_HEADER_GUARD
#define JCMR_APP_PROFILE_H_HEADER_GUARD

#include <chrono>
#include <string>

#include "log.h"

struct ProfileBlock {
    ProfileBlock(const char* name)
        : name(name)
        , start(std::chrono::high_resolution_clock::now())
    {
    }

    ~ProfileBlock()
    {
        auto end   = std::chrono::high_resolution_clock::now();
        auto total = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        LOG_INFO("[PROFILE] \"{}\" took {}ms", name, total.count());
    }

    std::string                           name;
    std::chrono::steady_clock::time_point start;
};

#endif // JCMR_APP_PROFILE_H_HEADER_GUARD

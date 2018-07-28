#pragma once

#include <memory.h>
#include <map>
#include <fnv1.h>

template <typename T>
class Factory
{
public:
    virtual std::string GetFactoryKey() const = 0;

    template <class... Args>
    static std::shared_ptr<T> make(Args&&... args)
    {
        auto p = std::make_shared<T>(std::forward<Args>(args)...);

        const auto& key = p->GetFactoryKey();
        const auto hash = fnv_1_32::hash(key.c_str(), key.length());

        std::lock_guard<decltype(InstancesMutex)> _lk{ InstancesMutex };
        Instances.insert(std::make_pair(hash, p));
        return p;
    }

    static size_t destroy(std::shared_ptr<T> p)
    {
        if (p) {
            const auto& key = p->GetFactoryKey();
            const auto hash = fnv_1_32::hash(key.c_str(), key.length());

            std::lock_guard<decltype(InstancesMutex)> _lk{ InstancesMutex };
            Instances.erase(hash);
        }
    }

    static std::recursive_mutex InstancesMutex;
    static std::map<uint32_t, std::shared_ptr<T>> Instances;
};
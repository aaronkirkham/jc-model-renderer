#pragma once

#include <fnv1.h>
#include <map>
#include <memory.h>

template <typename T> class Factory
{
  public:
    virtual std::string GetFactoryKey() const = 0;

    template <class... Args> static std::shared_ptr<T> make(Args&&... args)
    {
        auto p = std::make_shared<T>(std::forward<Args>(args)...);

        const auto& key  = p->GetFactoryKey();
        const auto  hash = fnv_1_32::hash(key.c_str(), key.length());

        std::lock_guard<decltype(InstancesMutex)> _lk{InstancesMutex};
        Instances.insert(std::make_pair(hash, p));
        return p;
    }

    static void destroy(std::shared_ptr<T> p)
    {
        if (p) {
            const auto& key  = p->GetFactoryKey();
            const auto  hash = fnv_1_32::hash(key.c_str(), key.length());

            std::lock_guard<decltype(InstancesMutex)> _lk{InstancesMutex};
            Instances.erase(hash);
        }
    }

    static std::shared_ptr<T> get(const std::string& key)
    {
        const auto hash = fnv_1_32::hash(key.c_str(), key.length());

        std::lock_guard<decltype(InstancesMutex)> _lk{InstancesMutex};
        const auto                                find_it =
            std::find_if(Instances.begin(), Instances.end(),
                         [&](const std::pair<uint32_t, std::shared_ptr<T>>& item) { return item.first == hash; });

        if (find_it != Instances.end()) {
            return (*find_it).second;
        }

        return nullptr;
    }

    static std::recursive_mutex                   InstancesMutex;
    static std::map<uint32_t, std::shared_ptr<T>> Instances;
};

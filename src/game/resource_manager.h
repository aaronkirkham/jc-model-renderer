#ifndef JCMR_GAME_RESOURCE_MANAGER_H_HEADER_GUARD
#define JCMR_GAME_RESOURCE_MANAGER_H_HEADER_GUARD

#include "platform.h"

namespace jcmr
{
struct App;
struct DirectoryList;

struct ResourceManager {
    using DecompressCallback_t = std::function<void(const ByteArray&)>;

    struct AsyncHandle {
        u32 value;

        explicit AsyncHandle(u32 value)
            : value(value)
        {
        }

        bool               valid() const { return value != 0xFFFFFFFF; }
        static AsyncHandle invalid() { return AsyncHandle(0xFFFFFFFF); }
    };

    enum Flags : u32 {
        E_FLAG_LEGACY_ARCHIVE_TABLE = (1 << 0),
    };

    static ResourceManager* create(App& app);
    static void             destroy(ResourceManager* instance);

    virtual ~ResourceManager() = default;

    virtual void           set_base_path(const std::filesystem::path& base_path) = 0;
    virtual void           set_flags(u32 flags)                                  = 0;
    virtual void           load_dictionary(i32 resource_id)                      = 0;
    virtual DirectoryList& get_dictionary_tree()                                 = 0;

    // virtual void        process_callbacks()                                                    = 0;
    // virtual AsyncHandle decompress(const std::string& filename, DecompressCallback_t callback) = 0;

    virtual bool read(u32 namehash, ByteArray* out_buffer)                          = 0;
    virtual bool read(const std::string& filename, ByteArray* out_buffer)           = 0;
    virtual bool read_from_disk(const std::string& filename, ByteArray* out_buffer) = 0;
};
} // namespace jcmr

#endif // JCMR_GAME_RESOURCE_MANAGER_H_HEADER_GUARD

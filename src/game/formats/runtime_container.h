#ifndef JCMR_FORMATS_RUNTIME_CONTAINER_H_HEADER_GUARD
#define JCMR_FORMATS_RUNTIME_CONTAINER_H_HEADER_GUARD

#include "../format.h"

namespace jcmr
{
struct App;

namespace game::format
{
    struct RuntimeContainer : IFormat {
        using VariantData = std::tuple<std::string, bool, u32>;

        enum VariantDataType : u8 {
            VARIANT_DATATYPE_NAMEHASH = 0,
            VARIANT_DATATYPE_GUID     = 1,
            VARIANT_DATATYPE_COLOUR   = 2,

            VARIANT_DATATYPE_COUNT,
        };

        static RuntimeContainer* create(App& app);
        static void              destroy(RuntimeContainer* instance);

        std::vector<const char*> get_extensions() override { return {"epe", "blo"}; }
        u32                      get_header_magic() const override { return ava::RuntimePropertyContainer::RTPC_MAGIC; }
    };
} // namespace game::format
} // namespace jcmr

#endif // JCMR_FORMATS_RUNTIME_CONTAINER_H_HEADER_GUARD

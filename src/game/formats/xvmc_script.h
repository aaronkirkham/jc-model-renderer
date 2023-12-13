#ifndef JCMR_FORMATS_XVMC_SCRIPT_H_HEADER_GUARD
#define JCMR_FORMATS_XVMC_SCRIPT_H_HEADER_GUARD

#include "../format.h"

namespace jcmr
{
struct App;

namespace game::format
{
    struct XVMCScript : IFormat {
        static XVMCScript* create(App& app);
        static void        destroy(XVMCScript* instance);

        std::vector<const char*> get_extensions() override { return {"xvmc"}; }
    };
} // namespace game::format
} // namespace jcmr

#endif // JCMR_FORMATS_XVMC_SCRIPT_H_HEADER_GUARD

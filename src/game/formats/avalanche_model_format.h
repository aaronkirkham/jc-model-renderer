#ifndef JCMR_FORMATS_AVALANCHE_MODEL_FORMAT_H_HEADER_GUARD
#define JCMR_FORMATS_AVALANCHE_MODEL_FORMAT_H_HEADER_GUARD

#include "../format.h"

namespace jcmr
{
struct App;

namespace game::format
{
    struct AvalancheModelFormat : IFormat {
        static AvalancheModelFormat* create(App& app);
        static void                  destroy(AvalancheModelFormat* instance);

        std::vector<const char*> get_extensions() override { return {"modelc"}; }
    };

} // namespace game::format
} // namespace jcmr

#endif // JCMR_FORMATS_AVALANCHE_MODEL_FORMAT_H_HEADER_GUARD

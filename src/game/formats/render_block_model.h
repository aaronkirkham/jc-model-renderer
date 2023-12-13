#ifndef JCMR_FORMATS_RENDER_BLOCK_MODEL_H_HEADER_GUARD
#define JCMR_FORMATS_RENDER_BLOCK_MODEL_H_HEADER_GUARD

#include "../format.h"

namespace jcmr
{
struct App;

namespace game::format
{
    struct RenderBlockModel : IFormat {
        static RenderBlockModel* create(App& app);
        static void              destroy(RenderBlockModel* instance);

        std::vector<const char*> get_extensions() override { return {"rbm", "lod"}; }
        // u32 get_header_magic() const override { ava::RenderBlockModel::MAGIC }
    };

} // namespace game::format
} // namespace jcmr

#endif // JCMR_FORMATS_RENDER_BLOCK_MODEL_H_HEADER_GUARD

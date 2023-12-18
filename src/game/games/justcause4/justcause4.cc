#include "pch.h"

#include "justcause4.h"

#include "app/app.h"
#include "app/settings.h"

#include "game/games/justcause4/renderblocks/renderblockcharacter.h"
#include "game/name_hash_lookup.h"
#include "game/render_block.h"
#include "game/resource_manager.h"

#include <AvaFormatLib/archives/oodle_helper.h>

#include <fstream>

namespace jcmr::game
{
static constexpr i32 INTERNAL_RESOURCE_JUSTCAUSE4_DICTIONARY = 256;

struct JustCause4Impl final : JustCause4 {
    JustCause4Impl(App& app)
        : m_app(app)
    {
        m_directory = m_app.get_settings().get<const char*>("jc4_path");

        m_resource_manager = ResourceManager::create(app);
        m_resource_manager->set_base_path(m_directory);
        m_resource_manager->load_dictionary(INTERNAL_RESOURCE_JUSTCAUSE4_DICTIONARY);

        auto oodle_lib_path = fmt::format("{}\\oo2core_7_win64.dll", m_directory.string());
        AVA_FL_ENSURE(ava::Oodle::LoadLib(oodle_lib_path.c_str()));

        // load shader bundle : ShadersDX11_F.shader_bundle

        // load_shader_bundle();
        // init_shader_constants();
    }

    ~JustCause4Impl()
    {
        ava::Oodle::UnloadLib();
        ResourceManager::destroy(m_resource_manager);
    }

    IRenderBlock* create_render_block(u32 typehash) override
    {
        using namespace jcmr::game::justcause4;
        using namespace ava::RenderBlockModel;

        switch (typehash) {
            case RB_CHARACTER: return RenderBlockCharacter::create(m_app);
        }

        return nullptr;
    }

    std::shared_ptr<Texture> create_texture(const std::string& filename) override
    {
        // TODO
        return nullptr;
    }

    std::shared_ptr<Shader> create_shader(const std::string& filename, u8 shader_type) override
    {
        // TODO
        return nullptr;
    }

    const std::filesystem::path& get_directory() const override { return m_directory; }
    ResourceManager*             get_resource_manager() override { return m_resource_manager; }

  private:
    App&                  m_app;
    std::filesystem::path m_directory;
    ResourceManager*      m_resource_manager = nullptr;
};

JustCause4* JustCause4::create(App& app)
{
    return new JustCause4Impl(app);
}

void JustCause4::destroy(JustCause4* inst)
{
    ASSERT(inst);
    delete inst;
}
} // namespace jcmr::game
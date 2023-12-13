#include "pch.h"

#include "justcause4.h"

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
        LOG_INFO("JustCause4Impl");

        m_resource_manager = ResourceManager::create(app);
        m_resource_manager->set_base_path(m_directory);
        m_resource_manager->load_dictionary(INTERNAL_RESOURCE_JUSTCAUSE4_DICTIONARY);

        auto oodle_lib_path = fmt::format("{}\\oo2core_7_win64.dll", m_directory.string());
        LOG_INFO("oodle_lib_path: {}", oodle_lib_path);

        // E:\\Epic Games\\JustCause4\\oo2core_7_win64.dll
        AVA_FL_ENSURE(ava::Oodle::LoadLib(oodle_lib_path.c_str()));

        // load shader bundle : ShadersDX11_F.shader_bundle

        // test_load();
    }

    ~JustCause4Impl()
    {
        ava::Oodle::UnloadLib();
        ResourceManager::destroy(m_resource_manager);
    }

    void test_load()
    {
        using namespace ava;

        ByteArray buffer;
        if (!m_resource_manager->read("text/master_eng.stringlookup", &buffer)) {
            DEBUG_BREAK();
            return;
        }

        // std::vector<StreamArchive::ArchiveEntry> entries;
        // AVA_FL_ENSURE(StreamArchive::Parse(buffer, &entries));

        // for (const auto& entry : entries) {
        //     LOG_INFO("{} [{:x}] @ {} {}", entry.m_Filename, entry.m_NameHash, entry.m_Offset, entry.m_Size);
        // }

        // auto it = std::find_if(entries.begin(), entries.end(), [](const StreamArchive::ArchiveEntry& entry) {
        //     return entry.m_Filename
        //            == "editor/entities/characters/combatants/rebels/sargento_quest_rebels/"
        //               "sargentos_rebel_female_02.epe";
        // });

        // ASSERT(it != entries.end());
        // auto entry = (*it);

        // ByteArray entry_buffer;
        // AVA_FL_ENSURE(StreamArchive::ReadEntry(buffer, entry, &entry_buffer));

        LOG_INFO("all done :-)");
        exit(1);
    }

    IRenderBlock* create_render_block(u32 typehash) override
    {
        using namespace ava::RenderBlockModel;
        switch (typehash) {
            case RB_CHARACTER: return justcause4::RenderBlockCharacter::create(m_app);
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
    std::filesystem::path m_directory        = "E:\\SteamLibrary\\steamapps\\common\\Just Cause 4";
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
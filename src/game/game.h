#ifndef JCMR_GAME_GAME_H_HEADER_GUARD
#define JCMR_GAME_GAME_H_HEADER_GUARD

#include "platform.h"

#include "app/directory_list.h"

namespace ava::AvalancheTexture
{
struct TextureEntry;
}

namespace jcmr
{
struct App;
struct ResourceManager;
struct Texture;
struct Shader;
struct RenderContext;

namespace game
{
    struct IRenderBlock;
};

enum class EGame {
    EGAME_JUSTCAUSE3,
    EGAME_JUSTCAUSE4,
    EGAME_COUNT,
};

struct IGame {
    static IGame* create(EGame game_type, App& app);
    static void   destroy(IGame* game);

    virtual ~IGame() = default;

    virtual game::IRenderBlock*      create_render_block(u32 typehash)                          = 0;
    virtual std::shared_ptr<Texture> create_texture(const std::string& filename)                = 0;
    virtual std::shared_ptr<Texture> create_texture(const ava::AvalancheTexture::TextureEntry& entry,
                                                    const ByteArray&                           buffer)                    = 0;
    virtual std::shared_ptr<Texture> create_raw_texture(const std::string& filename)            = 0;
    virtual std::shared_ptr<Shader>  create_shader(const std::string& filename, u8 shader_type) = 0;

    using TextureEntryBufferPair = std::pair<ava::AvalancheTexture::TextureEntry, ByteArray>;
    virtual std::vector<TextureEntryBufferPair> load_textures(const std::string&                 filename,
                                                              ava::AvalancheTexture::AvtxHeader* out_header) = 0;

    virtual void setup_render_constants(RenderContext& context) {}

    virtual const char*                  get_title() const     = 0;
    virtual const std::filesystem::path& get_directory() const = 0;

    virtual ResourceManager* get_resource_manager() = 0;

    DirectoryList  m_dropzone_tree;
    void           load_dropzone(const std::filesystem::path& path);
    DirectoryList& get_dropzone_tree() { return m_dropzone_tree; }
};
} // namespace jcmr

#endif // JCMR_GAME_GAME_H_HEADER_GUARD

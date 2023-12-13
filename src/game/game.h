#ifndef JCMR_GAME_GAME_H_HEADER_GUARD
#define JCMR_GAME_GAME_H_HEADER_GUARD

#include "platform.h"

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
    virtual std::shared_ptr<Shader>  create_shader(const std::string& filename, u8 shader_type) = 0;

    virtual void setup_render_constants(RenderContext& context) {}

    virtual const std::filesystem::path& get_directory() const = 0;

    virtual ResourceManager* get_resource_manager() = 0;
};
} // namespace jcmr

#endif // JCMR_GAME_GAME_H_HEADER_GUARD

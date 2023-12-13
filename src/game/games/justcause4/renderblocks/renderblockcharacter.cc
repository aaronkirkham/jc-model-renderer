#include "pch.h"

#include "renderblockcharacter.h"

#include "app/app.h"
#include "app/profile.h"

#include "game/game.h"
#include "game/resource_manager.h"

#include "render/imguiex.h"
#include "render/renderer.h"
#include "render/shader.h"
#include "render/texture.h"

#include <AvaFormatLib/util/byte_array_buffer.h>
#include <AvaFormatLib/util/byte_vector_stream.h>
#include <d3d11.h>

namespace jcmr::game::justcause4
{
struct RenderBlockCharacterImpl final : RenderBlockCharacter {
    static constexpr u32 ATTR_TYPE_CHARACTER_CONSTANTS      = 0x25970695;
    static constexpr u32 ATTR_TYPE_CHARACTER_EYES_CONSTANTS = 0x1BAC0639;
    static constexpr u32 ATTR_TYPE_CHARACTER_HAIR_CONSTANTS = 0x342303CE;
    static constexpr u32 ATTR_TYPE_CHARACTER_SKIN_CONSTANTS = 0xD8749464;

  public:
    RenderBlockCharacterImpl(App& app)
        : RenderBlockCharacter(app)
    {
    }

    void read(const ava::SAdfDeferredPtr& attributes) override
    {
        // struct SAmfMaterial {
        //     uint32_t            m_Name;
        //     uint32_t            m_RenderBlockId;
        //     SAdfDeferredPtr     m_Attributes;
        //     SAdfArray<uint32_t> m_Textures;
        // };

        LOG_INFO("RenderBlockCharacter : read {:x}", attributes.m_Type);

        if (attributes.m_Type == ATTR_TYPE_CHARACTER_CONSTANTS) {
            LOG_INFO("ATTR_TYPE_CHARACTER_CONSTANTS");
        } else if (attributes.m_Type == ATTR_TYPE_CHARACTER_EYES_CONSTANTS) {
            LOG_INFO("ATTR_TYPE_CHARACTER_EYES_CONSTANTS");
        } else if (attributes.m_Type == ATTR_TYPE_CHARACTER_HAIR_CONSTANTS) {
            LOG_INFO("ATTR_TYPE_CHARACTER_HAIR_CONSTANTS");
        } else if (attributes.m_Type == ATTR_TYPE_CHARACTER_SKIN_CONSTANTS) {
            LOG_INFO("ATTR_TYPE_CHARACTER_SKIN_CONSTANTS");
        } else {
            LOG_ERROR("RenderBlockCharacter : unknown attribute types {:x}", attributes.m_Type);
            DEBUG_BREAK();
        }
    }

  private:
    //
};

RenderBlockCharacter* RenderBlockCharacter::create(App& app)
{
    return new RenderBlockCharacterImpl(app);
}

void RenderBlockCharacter::destroy(RenderBlockCharacter* instance)
{
    delete instance;
}
} // namespace jcmr::game::justcause4
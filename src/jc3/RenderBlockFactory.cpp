#include <jc3/RenderBlockFactory.h>
#include <jc3/renderblocks/RenderBlockGeneralJC3.h>
#include <jc3/renderblocks/RenderBlockCharacter.h>
#include <jc3/renderblocks//RenderBlockCharacterSkin.h>

IRenderBlock* RenderBlockFactory::CreateRenderBlock(const uint32_t type) {
    if (type == 0x2EE0F4A9) {
        return new RenderBlockGeneralJC3;
    }
    else if (type == 0x9D6E332A) {
        return new RenderBlockCharacter;
    }
    else if (type == 0x626F5E3B) {
        return new RenderBlockCharacterSkin;
    }
    
    return nullptr;
}
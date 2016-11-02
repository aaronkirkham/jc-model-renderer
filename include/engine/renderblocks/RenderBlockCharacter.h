#ifndef RENDERBLOCKCHARACTER_H
#define RENDERBLOCKCHARACTER_H

#include <MainWindow.h>

#pragma pack(push, 1)
struct CharacterAttributes
{
    uint32_t flags;
    float scale;
    char pad[0x64];
};

struct Character
{
    uint8_t version;
    CharacterAttributes attributes;
};
#pragma pack(pop)

class RenderBlockCharacter : public IRenderBlock
{
public:
    RenderBlockCharacter() = default;
    virtual ~RenderBlockCharacter() = default;

    virtual void Read(QDataStream& data) override
    {
        Reset();

        // read general block info
        Character block;
        RBMLoader::instance()->Read(data, block);

        // read materials
        m_Materials.Read(data);

        // TODO: this reads a weird vertex buffer. need to modify the current vertex buffer reader so we can specify a stride to read.
    }
};

#endif // RENDERBLOCKCHARACTER_H

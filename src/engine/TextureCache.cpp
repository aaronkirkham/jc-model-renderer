#include <MainWindow.h>

TextureCache::~TextureCache()
{
    m_CachedTextures.clear();
    m_CachedTexturesRefCount.clear();
}

QImage* TextureCache::GetTexture(const QString& filename)
{
    auto hash = qHash(filename);

    if (m_CachedTextures.find(hash) == m_CachedTextures.end())
        return CacheTexture(filename);

    qDebug() << "Using cached texture" << filename;
    m_CachedTexturesRefCount[hash]++;
    return &m_CachedTextures[hash];
}

// TODO: Give the textures a "1 file" lifetime so if we open another file it will still be cached until it's closed next
// most things use the same shared textures
void TextureCache::FreeTexture(const QString& filename)
{
    auto hash = qHash(filename);

    if (m_CachedTextures.find(hash) == m_CachedTextures.end())
        return;

    m_CachedTexturesRefCount[hash]--;
    if (m_CachedTexturesRefCount[hash] == 0)
    {
        m_CachedTexturesRefCount.remove(hash);
        m_CachedTextures.remove(hash);

        qDebug() << "Removed texture" << filename << "from cache.";
    }
}

struct TextureElement
{
    uint32_t offset;
    uint32_t size;
    uint16_t unknown;
    uint8_t unknown2;
    bool external;
};

QImage* TextureCache::CacheTexture(const QString& filename)
{
    qDebug() << "Caching texture" << filename;

    static uint32_t headerLength = 0x80;
    static uint32_t elementsCount = 8;

    QFile file(filename);
    if (file.open(QIODevice::ReadOnly))
    {
        DDSC ddscFile;
        file.read((char *)&ddscFile, sizeof(DDSC));

        if (ddscFile.magic == 0x58545641)
        {
            TextureElement elements[8];
            uint32_t biggestIndex = 0, biggestSize = 0;

            for (uint32_t i = 0; i < elementsCount; ++i)
            {
                // read the current element
                file.read((char *)&elements[i], sizeof(TextureElement));

                if (elements[i].size == 0)
                    continue;

                if ((false && elements[i].external) || (elements[i].external == false) && elements[i].size > biggestSize)
                {
                    biggestSize = elements[i].size;
                    biggestIndex = i;
                }
            }

            // find the texture rank
            uint32_t rank = 0;
            for (uint32_t i = 0; i < elementsCount; ++i)
            {
                if (i == biggestIndex)
                    continue;

                if (elements[i].size > elements[biggestIndex].size)
                    ++rank;
            }

            auto length = (file.size() - headerLength);
            auto block = new char[length];

            file.seek(headerLength);
            file.read(block, length);

            // write the texture data
            QByteArray textureData;
            QDataStream textureDataStream(&textureData, QIODevice::WriteOnly);
            textureDataStream.setByteOrder(QDataStream::LittleEndian);

            DDSHeader header;
            ZeroMemory(&header, sizeof(DDSHeader));
            header.magic = 0x20534444;
            header.size = 124;
            header.flags = 0x1007 | 0x20000;
            header.width = ddscFile.width >> rank;
            header.height = ddscFile.height >> rank;
            header.pitchOrLinearSize = 0;
            header.depth = ddscFile.depth;
            header.mipMapCount = 1;
            header.pixelFormat = GetDDCSPixelFormat(ddscFile.format);
            header.caps = 8 | 0x1000;
            header.caps2 = 0;

            textureDataStream.writeRawData((char *)&header, sizeof(DDSHeader));
            textureDataStream.writeRawData(block, length);

            delete[] block;

            auto hash = qHash(filename);
            auto texture = QGLWidget::convertToGLFormat(QImage::fromData(textureData).mirrored().rgbSwapped());

            m_CachedTextures[hash] = texture;
            m_CachedTexturesRefCount[hash] = 1;

            return &m_CachedTextures[hash];
        }

        file.close();
    }
    else
        qDebug() << "TextureCache::CacheTexture: Failed to load file" << filename;

    return nullptr;
}

DDSPixelFormat TextureCache::GetDDCSPixelFormat(uint32_t& format)
{
    DDSPixelFormat pixelFormat;
    pixelFormat.size = (8 * 4);

    switch (format)
    {
    case 71: // DXGI_FORMAT_BC1_UNORM
        pixelFormat.flags = DDSPixelFormat::FlagFourCC;
        pixelFormat.rgbBitCount = 0;
        pixelFormat.rBitMask = 0;
        pixelFormat.gBitMask = 0;
        pixelFormat.bBitMask = 0;
        pixelFormat.aBitMask = 0;
        pixelFormat.fourCC = Format::FormatDXT1;
        break;

    case 74: // DXGI_FORMAT_BC2_UNORM
        pixelFormat.flags = DDSPixelFormat::FlagFourCC;
        pixelFormat.rgbBitCount = 0;
        pixelFormat.rBitMask = 0;
        pixelFormat.gBitMask = 0;
        pixelFormat.bBitMask = 0;
        pixelFormat.aBitMask = 0;
        pixelFormat.fourCC = Format::FormatDXT3;
        break;

    case 77: // DXGI_FORMAT_BC3_UNORM
        pixelFormat.flags = DDSPixelFormat::FlagFourCC;
        pixelFormat.rgbBitCount = 0;
        pixelFormat.rBitMask = 0;
        pixelFormat.gBitMask = 0;
        pixelFormat.bBitMask = 0;
        pixelFormat.aBitMask = 0;
        pixelFormat.fourCC = Format::FormatDXT5;
        break;

    case 87: // DXGI_FORMAT_B8G8R8A8_UNORM
        pixelFormat.flags = DDSPixelFormat::FlagRGBA;
        pixelFormat.rgbBitCount = Format::FormatA8B8G8R8;
        pixelFormat.rBitMask = 0x00FF0000;
        pixelFormat.gBitMask = 0x0000FF00;
        pixelFormat.bBitMask = 0x000000FF;
        pixelFormat.aBitMask = 0xFF000000;
        pixelFormat.fourCC = 0;
        break;

    case 61: // DXGI_FORMAT_R8_UNORM
    case 80: // DXGI_FORMAT_BC4_UNORM
    case 83: // DXGI_FORMAT_BC5_UNORM
    case 98: // DXGI_FORMAT_BC7_UNORM
        pixelFormat.fourCC = 0x30315844;
        break;
    }

    return pixelFormat;
}

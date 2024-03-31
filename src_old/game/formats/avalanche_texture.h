#pragma once

#include "factory.h"

#include <array>
#include <filesystem>

namespace ava::AvalancheTexture
{
struct AvtxHeader;
};

class Texture;
class AvalancheTexture : public Factory<AvalancheTexture>
{
  private:
    std::filesystem::path                                                         m_Filename = "";
    ava::AvalancheTexture::AvtxHeader                                             m_Header   = {};
    std::array<std::unique_ptr<Texture>, ava::AvalancheTexture::AVTX_MAX_STREAMS> m_Textures = {nullptr};

  public:
    AvalancheTexture(const std::filesystem::path& filename);
    AvalancheTexture(const std::filesystem::path& filename, const std::vector<uint8_t>& buffer, bool external = false);
    virtual ~AvalancheTexture() = default;

    virtual std::string GetFactoryKey() const
    {
        return m_Filename.string();
    }

    void Parse(const std::vector<uint8_t>& buffer);

    static void ReadFileCallback(const std::filesystem::path& filename, const std::vector<uint8_t>& data,
                                 bool external);
    static bool SaveFileCallback(const std::filesystem::path& filename, const std::filesystem::path& path);

    void DrawUI();

    const std::filesystem::path& GetFilePath()
    {
        return m_Filename;
    }
};

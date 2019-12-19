#include <AvaFormatLib.h>
#include <imgui.h>

#include "avalanche_texture.h"
#include "util/byte_vector_writer.h"

#include "graphics/dds_texture_loader.h"
#include "graphics/imgui/fonts/fontawesome5_icons.h"
#include "graphics/imgui/imgui_buttondropdown.h"
#include "graphics/renderer.h"
#include "graphics/texture.h"
#include "graphics/texture_manager.h"
#include "graphics/ui.h"

#include "import_export/import_export_manager.h"

extern bool g_IsJC4Mode;

std::recursive_mutex                                  Factory<AvalancheTexture>::InstancesMutex;
std::map<uint32_t, std::shared_ptr<AvalancheTexture>> Factory<AvalancheTexture>::Instances;

AvalancheTexture::AvalancheTexture(const std::filesystem::path& filename)
    : m_Filename(filename)
{
    //
}

AvalancheTexture::AvalancheTexture(const std::filesystem::path& filename, const std::vector<uint8_t>& buffer,
                                   bool external)
    : m_Filename(filename)
{
    // parse AVTX
    Parse(buffer);
}

void AvalancheTexture::Parse(const std::vector<uint8_t>& buffer)
{
    using namespace ava::AvalancheTexture;

    std::memcpy(&m_Header, buffer.data(), sizeof(AvtxHeader));

#ifdef _DEBUG
    if (m_Header.m_Magic != AVTX_MAGIC) {
        __debugbreak();
    }

    if (m_Header.m_Version != 1) {
        __debugbreak();
    }
#endif

    for (uint32_t i = 0; i < ava::AvalancheTexture::AVTX_MAX_STREAMS; ++i) {
        const AvtxStream& avtx_stream = m_Header.m_Streams[i];

        if (avtx_stream.m_Size == 0 || avtx_stream.m_Source) {
            continue;
        }

        TextureEntry         entry{};
        std::vector<uint8_t> pixel_buffer;
        ava::AvalancheTexture::ReadEntry(buffer, i, &entry, &pixel_buffer);

        // generate the DDS header
        DDS_HEADER dds_header{};
        dds_header.size        = sizeof(DDS_HEADER);
        dds_header.flags       = (DDS_TEXTURE | DDSD_MIPMAPCOUNT);
        dds_header.width       = entry.m_Width;
        dds_header.height      = entry.m_Height;
        dds_header.depth       = entry.m_Depth;
        dds_header.mipMapCount = 1;
        dds_header.ddspf       = TextureManager::GetPixelFormat((DXGI_FORMAT)entry.m_Format);
        dds_header.caps        = (DDSCAPS_COMPLEX | DDSCAPS_TEXTURE);

        //
        bool isDXT10 = (dds_header.ddspf.fourCC == MAKEFOURCC('D', 'X', '1', '0'));

        // resize the final output buffer
        std::vector<uint8_t> dds_buffer(sizeof(DDS_MAGIC) + sizeof(DDS_HEADER)
                                        + (isDXT10 ? sizeof(DDS_HEADER_DXT10) : 0) + pixel_buffer.size());

        byte_vector_writer buf(&dds_buffer);
        buf.setp(0);

        // write the DDS header
        buf.write((char*)&DDS_MAGIC, sizeof(uint32_t));
        buf.write((char*)&dds_header, sizeof(DDS_HEADER));

        // write the DXT10 header
        if (isDXT10) {
            DDS_HEADER_DXT10 dxt10header{};
            dxt10header.dxgiFormat        = (DXGI_FORMAT)entry.m_Format;
            dxt10header.resourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE2D;
            dxt10header.arraySize         = 1;

            buf.write((char*)&dxt10header, sizeof(DDS_HEADER_DXT10));
        }

        // write the texture data
        buf.write((char*)pixel_buffer.data(), pixel_buffer.size());

        // create texture
        auto filename = m_Filename;
        filename += "-" + std::to_string(i);
        m_Textures[i] = std::make_unique<Texture>(filename, dds_buffer);
    }
}

void AvalancheTexture::ReadFileCallback(const std::filesystem::path& filename, const std::vector<uint8_t>& data,
                                        bool external)
{
    if (!AvalancheTexture::exists(filename.string())) {
        auto tex = AvalancheTexture::make(filename, data, external);
    }
}

bool AvalancheTexture::SaveFileCallback(const std::filesystem::path& filename, const std::filesystem::path& path)
{
    return false;
}

void AvalancheTexture::DrawUI()
{
    using namespace ava::AvalancheTexture;

    ImGui::Text("Tag: %d", m_Header.m_Tag);
    ImGui::Text("Dimension: %d", m_Header.m_Dimension);
    ImGui::Text("Format: %s", TextureManager::GetFormatString((DXGI_FORMAT)m_Header.m_Format));
    ImGui::Text("Width: %d", m_Header.m_Width);
    ImGui::Text("Height: %d", m_Header.m_Height);
    ImGui::Text("Depth: %d", m_Header.m_Depth);
    ImGui::Text("Flags: %d", m_Header.m_Flags);
    ImGui::InputScalar("Mips", ImGuiDataType_U8, &m_Header.m_Mips);
    ImGui::InputScalar("Mips Redisent", ImGuiDataType_U8, &m_Header.m_MipsRedisent);
    ImGui::Text("Mips Cinematic: %d", m_Header.m_MipsCinematic);
    ImGui::Text("Mips Bias: %d", m_Header.m_MipsBias);
    ImGui::Text("Lod Group: %d", m_Header.m_LodGroup);
    ImGui::Text("Pool: %d", m_Header.m_Pool);

    ImGui::Columns(4, nullptr, true);

    for (uint32_t i = 0; i < ava::AvalancheTexture::AVTX_MAX_STREAMS; ++i) {
        AvtxStream& stream = m_Header.m_Streams[i];

        ImGui::PushID(i);

        if (ImGui::GetColumnIndex() == 0) {
            ImGui::Separator();
        }

        ImGui::Text("Stream %d", (i + 1));

        // stream has no texture
        if (!stream.m_Size) {
            static ImVec4 red{1, 0, 0, 1};

            ImGui::SameLine();
            ImGui::TextColored(red, "(Empty)");
        }

        if (stream.m_Size) {
            const uint32_t rank   = GetRank(m_Header, i);
            uint32_t       width  = (m_Header.m_Width >> rank);
            uint32_t       height = (m_Header.m_Height >> rank);

            ImGui::Text("%d x %d", width, height);

            if (g_IsJC4Mode) {
                ImGui::InputScalar("Source", ImGuiDataType_U8, &stream.m_Source);
            } else {
                ImGui::Checkbox("Source", (bool*)&stream.m_Source);
            }

            ImGui::Checkbox("Tile Mode", &stream.m_TileMode);

            if (m_Textures[i]) {
                ImGui::Image(m_Textures[i]->GetSRV(), {100, 100});
            }
        } else {
            if (ImGuiCustom::BeginButtonDropDown("Import Texture From", ImVec2(-FLT_MIN, 0.0f))) {
                const auto& importers = ImportExportManager::Get()->GetImportersFor(".ddsc");
                for (const auto& importer : importers) {
                    if (ImGui::MenuItem(importer->GetExportName(), importer->GetExportExtension())) {
                        UI::Get()->Events().ImportFileRequest(
                            importer, [&, i](bool success, std::filesystem::path filename, std::any data) {
                                if (success) {
                                    auto& buffer  = std::any_cast<std::vector<uint8_t>>(data);
                                    m_Textures[i] = std::make_unique<Texture>(filename, buffer);

                                    m_Header.m_Streams[i].m_Offset    = 0;
                                    m_Header.m_Streams[i].m_Size      = buffer.size();
                                    m_Header.m_Streams[i].m_Alignment = 16;
                                    m_Header.m_Streams[i].m_TileMode  = false;
                                    m_Header.m_Streams[i].m_Source    = 0;

                                    // probably was the first texture added.
                                    // @TODO: make this better!
                                    if (m_Header.m_Format == 0) {
                                        const auto& desc = m_Textures[i]->GetDesc();

                                        m_Header.m_Format       = desc.Format;
                                        m_Header.m_Width        = desc.Width;
                                        m_Header.m_Height       = desc.Height;
                                        m_Header.m_Mips         = 9;
                                        m_Header.m_MipsRedisent = 9;
                                    }
                                }
                            });
                    }
                }

                ImGuiCustom::EndButtonDropDown();
            }
        }

        ImGui::PopID();
        ImGui::NextColumn();
    }

    ImGui::EndColumns();
    ImGui::Separator();
}

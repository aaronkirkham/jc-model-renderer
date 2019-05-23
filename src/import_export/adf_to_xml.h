#pragma once

#include <bitset>

#include "iimportexporter.h"

#include "../window.h"

#include "../game/file_loader.h"
#include "../game/formats/avalanche_data_format.h"

#include <tinyxml2.h>

namespace ImportExport
{

#define XmlPushAttribute(k, v)                                                                                         \
    if (v) {                                                                                                           \
        printer.PushAttribute(k, v);                                                                                   \
    }

class ADF2XML : public IImportExporter
{
  public:
    ADF2XML()          = default;
    virtual ~ADF2XML() = default;

    ImportExportType GetType() override final
    {
        return ImportExportType_Both;
    }

    const char* GetName() override final
    {
        return "XML";
    }

    std::vector<const char*> GetImportExtension() override final
    {
        // TODO: get a real list of all ADF types
        return {".adf", ".epe_adf", ".vmodc", ".wtunec", ".meshc", ".hrmeshc", ".modelc"};
    }

    const char* GetExportExtension() override final
    {
        return ".xml";
    }

    void Import(const std::filesystem::path& filename, ImportFinishedCallback callback) override final
    {
        return;
    }

    void XmlWriteInstance(::AvalancheDataFormat* adf, tinyxml2::XMLPrinter& printer,
                          const jc::AvalancheDataFormat::Header* header, jc::AvalancheDataFormat::Type* type,
                          const char* payload, uint32_t offset = 0, const char* name = nullptr)
    {
        using namespace jc::AvalancheDataFormat;

        switch (type->m_AdfType) {
            case EAdfType::Primitive: {
                switch (type->m_ScalarType) {
                    case ScalarType::Signed: {
                        if (type->m_Size == 1) {
                            printer.PushText(*(int8_t*)&payload[offset]);
                        } else if (type->m_Size == 2) {
                            printer.PushText(*(int16_t*)&payload[offset]);
                        } else if (type->m_Size == 4) {
                            printer.PushText(*(int32_t*)&payload[offset]);
                        } else if (type->m_Size == 8) {
                            printer.PushText(*(int64_t*)&payload[offset]);
                        }

                        break;
                    }

                    case ScalarType::Unsigned: {
                        if (type->m_Size == 1) {
                            printer.PushText(*(uint8_t*)&payload[offset]);
                        } else if (type->m_Size == 2) {
                            printer.PushText(*(uint16_t*)&payload[offset]);
                        } else if (type->m_Size == 4) {
                            printer.PushText(*(uint32_t*)&payload[offset]);
                        } else if (type->m_Size == 8) {
                            printer.PushText(*(uint64_t*)&payload[offset]);
                        }

                        break;
                    }

                    case ScalarType::Float: {
                        if (type->m_Size == 4) {
                            printer.PushText(*(float*)&payload[offset]);
                        } else if (type->m_Size == 8) {
                            printer.PushText(*(double*)&payload[offset]);
                        }

                        break;
                    }
                }

                break;
            }

            case EAdfType::Structure: {
                printer.OpenElement("struct");
                printer.PushAttribute("type", adf->m_Strings[type->m_Name].c_str());
                /*if (name) {
                    printer.PushAttribute("name", name);
                }*/

                for (uint32_t i = 0; i < type->m_MemberCount; ++i) {
                    const auto current_member = (Member*)((char*)type + sizeof(Type) + (sizeof(Member) * i));
                    const auto member_type    = adf->FindType(current_member->m_TypeHash);

                    printer.OpenElement("member");
                    printer.PushAttribute("name", adf->m_Strings[current_member->m_Name].c_str());

                    if (member_type->m_AdfType == EAdfType::BitField) {
                        auto data     = *(uint64_t*)&payload[offset];
                        auto bitfield = std::bitset<64>(data);
                        printer.PushText(bitfield.test(current_member->m_BitOffset));
                    } else {
                        XmlWriteInstance(adf, printer, header, member_type, payload,
                                         (offset + (current_member->m_Offset & 0xFFFFFF)));
                    }

                    printer.CloseElement();
                }

                printer.CloseElement();
                break;
            }

            case EAdfType::Pointer:
            case EAdfType::Deferred: {
                const auto data_offset = *(uint32_t*)&payload[offset];
                uint32_t   type_hash   = 0xDEFE88ED;

                if (type->m_AdfType == EAdfType::Pointer) {
                    type_hash = type->m_SubTypeHash;
                } else if (data_offset) {
                    type_hash = *(uint32_t*)&payload[offset + 8];
                }

                const auto deferred_type = adf->FindType(type_hash);
                if (deferred_type) {
                    printer.OpenElement("pointer");
                    printer.PushAttribute("type", type_hash);

                    XmlWriteInstance(adf, printer, header, deferred_type, payload, data_offset);

                    printer.CloseElement();
                }

                break;
            }

            case EAdfType::Array: {
                printer.OpenElement("array");

                const auto read_offset = *(uint32_t*)&payload[offset];
                const auto count       = *(uint32_t*)&payload[offset + 8];

                const auto subtype = adf->FindType(type->m_SubTypeHash);
                if (false && subtype->m_AdfType == EAdfType::Primitive /* && count > 64 */) {
                    if (count > 0) {
                        /*const auto& data =
                            base64_encode((unsigned char*)&payload[read_offset], (subtype->m_Size * count));
                        printer.PushText(data.c_str());*/
                    }
                } else {
                    for (uint32_t i = 0; i < count; ++i) {
                        XmlWriteInstance(adf, printer, header, subtype, payload, (read_offset + (subtype->m_Size * i)));

                        // if it's not the last element, add a space
                        /*if (subtype->m_AdfType == EAdfType::Primitive && (i != (count - 1))) {
                            printer.PushText(" ");
                        }*/
                    }
                }

                printer.CloseElement();
                break;
            }

            case EAdfType::InlineArray: {
                printer.OpenElement("inline_array");

                const auto subtype = adf->FindType(type->m_SubTypeHash);
                auto       size    = subtype->m_Size;
                if (subtype->m_AdfType == EAdfType::String || subtype->m_AdfType == EAdfType::Pointer) {
                    size = 8;
                }

                for (uint32_t i = 0; i < type->m_ArraySizeOrBitCount; ++i) {
                    XmlWriteInstance(adf, printer, header, subtype, payload, offset + (size * i));

                    // if it's not the last element, add a space
                    if (subtype->m_AdfType == EAdfType::Primitive && i != (type->m_ArraySizeOrBitCount - 1)) {
                        printer.PushText(" ");
                    }
                }

                printer.CloseElement();
                break;
            }

            case EAdfType::String: {
                // TODO: Only if header flag & 1 is set? (has relative offsets)
                auto string_offset = *(uint32_t*)&payload[offset];
                auto value         = (const char*)&payload[string_offset];
                printer.PushText(value);
                break;
            }

            case EAdfType::Enumeration: {
                printer.PushComment("Enumeration");
                // TODO
                break;
            }

            case EAdfType::StringHash: {
                printer.OpenElement("stringhash");

                auto        string_hash = *(uint32_t*)&payload[offset];
                const auto& string      = adf->HashLookup(string_hash);

                printer.PushText(string);
                printer.CloseElement();
                break;
            }
        }
    }

    void DoExport(const std::filesystem::path& path, ::AvalancheDataFormat* adf)
    {
        using namespace jc::AvalancheDataFormat;

        const auto& buffer = adf->GetBuffer();

        Header      header_out{};
        bool        byte_swap_out;
        const char* description_out = "";
        adf->ParseHeader((Header*)buffer.data(), buffer.size(), &header_out, &byte_swap_out, &description_out);

        tinyxml2::XMLPrinter printer;
        printer.PushHeader(false, true);
        printer.PushComment(" File generated by " VER_PRODUCTNAME_STR " v" VER_PRODUCT_VERSION_STR " ");
        printer.PushComment(" https://github.com/aaronkirkham/jc-model-renderer ");

        printer.OpenElement("adf");
        XmlPushAttribute("extension", adf->GetFilename().extension().string().c_str());
        XmlPushAttribute("version", header_out.m_Version);
        XmlPushAttribute("flags", header_out.m_Flags);

        if (strlen(description_out) > 0) {
            printer.PushAttribute("library", description_out);
        }

        // instances
        auto instance_buffer = &buffer[header_out.m_FirstInstanceOffset];
        for (uint32_t i = 0; i < header_out.m_InstanceCount; ++i) {
            auto current_instance = (Instance*)instance_buffer;

            const auto& instance_name = adf->m_Strings[current_instance->m_Name];

            printer.OpenElement("instance");
            XmlPushAttribute("name", instance_name.c_str());
            XmlPushAttribute("namehash", current_instance->m_NameHash);
            XmlPushAttribute("typehash", current_instance->m_TypeHash);
            XmlPushAttribute("offset", current_instance->m_PayloadOffset);
            XmlPushAttribute("size", current_instance->m_PayloadSize);

            // write the type members
            const auto type = adf->FindType(current_instance->m_TypeHash);
            if (type) {
                XmlWriteInstance(adf, printer, &header_out, type,
                                 (const char*)&buffer[current_instance->m_PayloadOffset], 0, instance_name.c_str());
            }

            printer.CloseElement();

            instance_buffer += ((header_out.m_Version > 3) ? sizeof(Instance) : sizeof(InstanceV3));
        }

        // types
        if (header_out.m_TypeCount > 0) {
            printer.PushComment(" You should really leave these alone! ");
            printer.OpenElement("types");

            auto types_buffer = &adf->m_Buffer[header_out.m_FirstTypeOffset];
            for (uint32_t i = 0; i < header_out.m_TypeCount; ++i) {
                const auto current_type = (Type*)types_buffer;
                const auto size =
                    current_type->m_MemberCount
                        * ((current_type->m_AdfType == EAdfType::Enumeration) ? sizeof(Enum) : sizeof(Member))
                    + sizeof(Type);

                printer.OpenElement("type");
                XmlPushAttribute("name", adf->m_Strings[current_type->m_Name].c_str());
                XmlPushAttribute("type", static_cast<uint32_t>(current_type->m_AdfType));
                XmlPushAttribute("size", current_type->m_Size);
                XmlPushAttribute("alignment", current_type->m_Align);
                XmlPushAttribute("typehash", current_type->m_TypeHash);
                XmlPushAttribute("flags", current_type->m_Flags);
                XmlPushAttribute("scalartype", static_cast<uint32_t>(current_type->m_ScalarType));
                XmlPushAttribute("subtypehash", current_type->m_SubTypeHash);
                XmlPushAttribute("arraysizeorbitcount", current_type->m_ArraySizeOrBitCount);

                const auto is_enum = (current_type->m_AdfType == EAdfType::Enumeration);
                if (!is_enum) {
                    for (uint32_t y = 0; y < current_type->m_MemberCount; ++y) {
                        const auto current_member = (Member*)(types_buffer + sizeof(Type) + (sizeof(Member) * y));
                        printer.OpenElement("member");
                        XmlPushAttribute("name", adf->m_Strings[current_member->m_Name].c_str());
                        XmlPushAttribute("typehash", current_member->m_TypeHash);
                        XmlPushAttribute("alignment", current_member->m_Align);
                        XmlPushAttribute("bitoffset", current_member->m_BitOffset);
                        XmlPushAttribute("flags", current_member->m_Flags);
                        XmlPushAttribute("default", static_cast<int64_t>(current_member->m_DefaultValue));
                        printer.CloseElement();
                    }
                }
                printer.CloseElement();

                // next element
                types_buffer += size;
            }

            printer.CloseElement();
        }

        printer.CloseElement();

        // write
        std::ofstream out_stream(path);
        out_stream << printer.CStr();
        out_stream.close();
    }

    void Export(const std::filesystem::path& filename, const std::filesystem::path& to,
                ExportFinishedCallback callback) override final
    {
        auto& path = to / filename.filename();
        path += GetExportExtension();
        SPDLOG_INFO("Exporting ADF to \"{}\"", path.string());

        auto adf = ::AvalancheDataFormat::get(filename.string());
        if (adf) {
            DoExport(path, adf.get());
            callback(true);
        } else {
            FileLoader::Get()->ReadFile(filename, [&, filename, path, callback](bool success, FileBuffer data) {
                if (success) {
                    auto adf = std::make_unique<::AvalancheDataFormat>(filename, data);
                    DoExport(path, adf.get());
                }

                callback(success);
            });
        }
    }
};

}; // namespace ImportExport

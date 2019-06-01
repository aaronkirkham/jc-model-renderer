#pragma once

#include <bitset>

#include "iimportexporter.h"

#include "../util.h"
#include "../window.h"

#include "../game/file_loader.h"
#include "../game/formats/avalanche_data_format.h"
#include "../game/hashlittle.h"

#include <tinyxml2.h>

namespace ImportExport
{

#define XmlPushAttribute(k, v)                                                                                         \
    if (v) {                                                                                                           \
        printer.PushAttribute(k, v);                                                                                   \
    }

#define VectorPushIfNotExists(container, value)                                                                        \
    if (std::find(container.begin(), container.end(), value) == container.end()) {                                     \
        container.push_back(value);                                                                                    \
    }

class ADF2XML : public IImportExporter
{
  private:
    std::map<std::string, uint32_t> m_StringRefs;
    std::vector<uint32_t>           m_RefsToFix;

    void PrefetchStringHashes(::AvalancheDataFormat* adf, tinyxml2::XMLElement* el)
    {
        if (!strcmp(el->Name(), "stringhash")) {
            const auto text           = el->GetText();
            const auto hash           = hashlittle(text);
            adf->m_StringHashes[hash] = text;
        }

        for (auto child = el->FirstChildElement(); child != nullptr; child = child->NextSiblingElement()) {
            PrefetchStringHashes(adf, child);
        }
    }

    void PushHigherPrecisionFloat(tinyxml2::XMLPrinter& printer, float value)
    {
        static auto TIXML_SNPRINTF = [](char* buffer, size_t size, const char* format, ...) {
            va_list va;
            va_start(va, format);
            vsnprintf_s(buffer, size, _TRUNCATE, format, va);
            va_end(va);
        };

        char buf[200];
        TIXML_SNPRINTF(buf, 200, "%.9g", value);
        printer.PushText(buf, false);
    }

    void WriteFromString(const jc::AvalancheDataFormat::Type* type, const char* payload, uint32_t offset,
                         const std::string& data)
    {
        using namespace jc::AvalancheDataFormat;

        if (type->m_ScalarType == ScalarType::Signed) {
            if (type->m_Size == 1) {
                auto value = static_cast<int8_t>(std::stoi(data));
                std::memcpy((char*)payload + offset, &value, sizeof(value));
            } else if (type->m_Size == 2) {
                auto value = static_cast<int16_t>(std::stoi(data));
                std::memcpy((char*)payload + offset, &value, sizeof(value));
            } else if (type->m_Size == 4) {
                auto value = static_cast<int32_t>(std::stol(data));
                std::memcpy((char*)payload + offset, &value, sizeof(value));
            } else if (type->m_Size == 8) {
                auto value = static_cast<int64_t>(std::stoll(data));
                std::memcpy((char*)payload + offset, &value, sizeof(value));
            }
        } else if (type->m_ScalarType == ScalarType::Unsigned) {
            if (type->m_Size == 1) {
                auto value = static_cast<uint8_t>(std::stoi(data));
                std::memcpy((char*)payload + offset, &value, sizeof(value));
            } else if (type->m_Size == 2) {
                auto value = static_cast<uint16_t>(std::stoi(data));
                std::memcpy((char*)payload + offset, &value, sizeof(value));
            } else if (type->m_Size == 4) {
                auto value = static_cast<uint32_t>(std::stoul(data));
                std::memcpy((char*)payload + offset, &value, sizeof(value));
            } else if (type->m_Size == 8) {
                auto value = static_cast<uint64_t>(std::stoull(data));
                std::memcpy((char*)payload + offset, &value, sizeof(value));
            }
        } else if (type->m_ScalarType == ScalarType::Float) {
            if (type->m_Size == 4) {
                auto value = std::stof(data);
                std::memcpy((char*)payload + offset, &value, sizeof(value));
            } else if (type->m_Size == 8) {
                auto value = std::stod(data);
                std::memcpy((char*)payload + offset, &value, sizeof(value));
            }
        }
    }

  public:
    ADF2XML()          = default;
    virtual ~ADF2XML() = default;

    ImportExportType GetType() override final
    {
        return ImportExportType_Both;
    }

    const char* GetImportName() override final
    {
        return "Avalanche Data Format";
    }

    std::vector<std::string> GetImportExtension() override final
    {
        // TODO: get a real list of all ADF extensions
        return {".blo_adf", ".flo_adf", ".epe_adf", ".vmodc", ".wtunec", ".modelc", ".meshc", ".hrmeshc", ".environc"};
    }

    const char* GetExportName() override final
    {
        return "XML";
    }

    const char* GetExportExtension() override final
    {
        return ".xml";
    }

    bool IsDragDropImportable() override final
    {
        return true;
    }

    uint32_t XmlReadInstance(::AvalancheDataFormat* adf, tinyxml2::XMLElement* el,
                             const jc::AvalancheDataFormat::Type* type, const char* payload, uint32_t payload_offset,
                             uint32_t data_offset)
    {
        using namespace jc::AvalancheDataFormat;

        switch (type->m_AdfType) {
            case EAdfType::Primitive: {
                payload_offset = jc::ALIGN_TO_BOUNDARY(payload_offset, type->m_Align);

                switch (type->m_ScalarType) {
                    case ScalarType::Signed: {
                        if (type->m_Size == 1) {
                            auto value = static_cast<int8_t>(el->IntText());
                            std::memcpy((char*)payload + payload_offset, &value, sizeof(value));
                        } else if (type->m_Size == 2) {
                            auto value = static_cast<int16_t>(el->IntText());
                            std::memcpy((char*)payload + payload_offset, &value, sizeof(value));
                        } else if (type->m_Size == 4) {
                            auto value = el->IntText();
                            std::memcpy((char*)payload + payload_offset, &value, sizeof(value));
                        } else if (type->m_Size == 8) {
                            auto value = el->Int64Text();
                            std::memcpy((char*)payload + payload_offset, &value, sizeof(value));
                        }

                        break;
                    }

                    case ScalarType::Unsigned: {
                        if (type->m_Size == 1) {
                            auto value = static_cast<uint8_t>(el->UnsignedText());
                            std::memcpy((char*)payload + payload_offset, &value, sizeof(value));
                        } else if (type->m_Size == 2) {
                            auto value = static_cast<uint16_t>(el->UnsignedText());
                            std::memcpy((char*)payload + payload_offset, &value, sizeof(value));
                        } else if (type->m_Size == 4) {
                            auto value = el->UnsignedText();
                            std::memcpy((char*)payload + payload_offset, &value, sizeof(value));
                        } else if (type->m_Size == 8) {
                            auto value = el->Unsigned64Text();
                            std::memcpy((char*)payload + payload_offset, &value, sizeof(value));
                        }

                        break;
                    }

                    case ScalarType::Float: {
                        if (type->m_Size == 4) {
                            auto value = el->FloatText();
                            std::memcpy((char*)payload + payload_offset, &value, sizeof(value));
                        } else if (type->m_Size == 8) {
                            auto value = el->DoubleText();
                            std::memcpy((char*)payload + payload_offset, &value, sizeof(value));
                        }

                        break;
                    }
                }

                // NOTE: no return as we didn't write anything to the data offset
                break;
            }

            case EAdfType::Structure: {
                auto struct_el = el->FirstChildElement("struct");
                if (!strcmp(el->Name(), "struct")) {
                    struct_el = el;
                }

                if (struct_el) {
                    uint32_t   member_index        = 0;
                    uint32_t   member_offset       = 0;
                    uint32_t   current_data_offset = 0;
                    const auto member_el           = struct_el->FirstChildElement("member");

                    for (auto child = member_el; child != nullptr; child = child->NextSiblingElement("member")) {
                        // get the current member from the child index
                        const auto current_member =
                            (Member*)((char*)type + sizeof(Type) + (sizeof(Member) * member_index));
                        const auto member_type = adf->FindType(current_member->m_TypeHash);

                        // get the offset where to write the next element
                        // TODO: the payload offset should use ALIGN_TO_BOUNDARY, then we don't have to manually
                        // do it in each case for primitives/strings/hashes/enums etc.
                        const uint32_t real_payload_offset = (payload_offset + member_offset);
                        const uint32_t alignment = jc::DISTANCE_TO_BOUNDARY(real_payload_offset, member_type->m_Align);

                        // get the offset where we want to write the actual data
                        const uint32_t real_data_offset =
                            jc::ALIGN_TO_BOUNDARY(data_offset + current_data_offset, member_type->m_Align);

                        if (member_type->m_AdfType == EAdfType::BitField) {
                            // assert(member_type->m_Size == 4);

                            // get the current bit value
                            uint64_t value = 0;
                            if (member_type->m_ScalarType == ScalarType::Signed) {
                                value = child->Int64Text();
                            } else if (member_type->m_ScalarType == ScalarType::Unsigned) {
                                value = child->Unsigned64Text();
                            }

                            // TODO: we should generate bit_offset our selfs.
                            auto v7            = value & ((1 << member_type->m_ArraySizeOrBitCount) - 1);
                            auto current_value = *(uint32_t*)&payload[real_payload_offset];
                            *(uint32_t*)&payload[real_payload_offset] |= (v7 << current_member->m_BitOffset);

                            // update current member data offset
                            current_member->m_Offset = jc::ALIGN_TO_BOUNDARY(member_offset, type->m_Align);
                        } else {
                            // write instance
                            current_data_offset += XmlReadInstance(adf, child, member_type, payload,
                                                                   real_payload_offset, real_data_offset);

                            // update current member data offset
                            current_member->m_Offset = jc::ALIGN_TO_BOUNDARY(member_offset, member_type->m_Align);

                            // next member offset
                            member_offset += (member_type->m_Size + alignment);
                        }

                        ++member_index;
                    }

                    return current_data_offset;
                }

                break;
            }

            case EAdfType::Pointer:
            case EAdfType::Deferred: {
                auto pointer_el = el->FirstChildElement("pointer");
                if (!strcmp(el->Name(), "pointer")) {
                    pointer_el = el;
                }

                if (pointer_el) {
                    const auto type_hash     = pointer_el->UnsignedAttribute("type");
                    const auto deferred_type = adf->FindType(type_hash);

                    if (deferred_type && pointer_el->FirstChildElement()) {
                        // write the reference to the data
                        const uint32_t next_offset = -1;
                        payload_offset             = jc::ALIGN_TO_BOUNDARY(payload_offset, type->m_Align);
                        std::memcpy((char*)payload + payload_offset, &data_offset, sizeof(data_offset));
                        std::memcpy((char*)payload + (payload_offset + 4), &next_offset, sizeof(next_offset));
                        std::memcpy((char*)payload + (payload_offset + 8), &type_hash, sizeof(type_hash));

                        //
                        m_RefsToFix.push_back(payload_offset);

                        const auto deferred_payload_offset = (data_offset);
                        const auto deferred_data_offset    = (data_offset + deferred_type->m_Size);
                        auto       size = XmlReadInstance(adf, pointer_el->FirstChildElement(), deferred_type, payload,
                                                    deferred_payload_offset, deferred_data_offset);

                        return deferred_data_offset - (deferred_payload_offset - size);
                    }
                }

                break;
            }

            case EAdfType::Array: {
                if (const auto array_el = el->FirstChildElement("array")) {
                    // TODO: dont read/write array count. we should be able to figure this out from the type
                    const auto count = array_el->UnsignedAttribute("count");

                    // if we don't have any members, write zeros
                    if (count == 0) {
                        static uint32_t zero = 0;
                        payload_offset       = jc::ALIGN_TO_BOUNDARY(payload_offset, type->m_Align);
                        std::memcpy((char*)payload + payload_offset, &zero, sizeof(zero));
                        std::memcpy((char*)payload + (payload_offset + 4), &zero, sizeof(zero));
                        std::memcpy((char*)payload + (payload_offset + 8), &zero, sizeof(zero));

                        return 0;
                    }

                    uint32_t written = 0;

                    // write the array sub types
                    const auto subtype = adf->FindType(type->m_SubTypeHash);
                    assert(subtype);
                    if (subtype->m_AdfType == EAdfType::Primitive) {
                        // TODO: better support for binary data.

                        const auto& data = util::split(array_el->GetText(), " ");
                        for (uint32_t i = 0; i < count; ++i) {
                            WriteFromString(subtype, payload, (data_offset + (subtype->m_Size * i)), data[i]);
                        }

                        written = jc::ALIGN_TO_BOUNDARY((subtype->m_Size * count), type->m_Align);
                    } else {
                        // read array members
                        uint32_t   member_offset       = 0;
                        uint32_t   current_data_offset = 0;
                        const auto array_member_el     = array_el->FirstChildElement();
                        for (auto child = array_member_el; child != nullptr; child = child->NextSiblingElement()) {
                            // calculate payload and data offsets
                            const uint32_t member_payload_offset = (data_offset + member_offset);
                            const uint32_t member_data_offset =
                                data_offset + (subtype->m_Size * count) + current_data_offset;

                            // write the instance
                            current_data_offset += XmlReadInstance(adf, child, subtype, payload, member_payload_offset,
                                                                   member_data_offset);

                            // next
                            member_offset += subtype->m_Size;
                        }

                        written = jc::ALIGN_TO_BOUNDARY((subtype->m_Size * count) + current_data_offset, type->m_Align);
                    }

                    // write the reference to the data
                    const uint32_t next_offset = -1;
                    payload_offset             = jc::ALIGN_TO_BOUNDARY(payload_offset, type->m_Align);
                    std::memcpy((char*)payload + payload_offset, &data_offset, sizeof(data_offset));
                    std::memcpy((char*)payload + (payload_offset + 4), &next_offset, sizeof(next_offset));
                    std::memcpy((char*)payload + (payload_offset + 8), &count, sizeof(count));

                    //
                    m_RefsToFix.push_back(payload_offset);

                    return written;
                }

                break;
            }

            case EAdfType::InlineArray: {
                auto inline_array_el = el->FirstChildElement("inline_array");
                if (!strcmp(el->Name(), "inline_array")) {
                    inline_array_el = el;
                }

                if (inline_array_el) {
                    const auto subtype = adf->FindType(type->m_SubTypeHash);
                    auto       size    = subtype->m_Size;
                    if (subtype->m_AdfType == EAdfType::String || subtype->m_AdfType == EAdfType::Pointer) {
                        size = 8;
                    }

                    if (subtype->m_AdfType == EAdfType::Primitive) {
                        const auto& data = util::split(inline_array_el->GetText(), " ");

                        // NOTE: we dont want any new entries as the array is a fixed size.
                        // TODO: should show errors in jc-model-renderer when we add this.
                        assert(data.size() == type->m_ArraySizeOrBitCount);

                        for (uint32_t i = 0; i < type->m_ArraySizeOrBitCount; ++i) {
                            WriteFromString(subtype, payload, (payload_offset + (size * i)), data[i]);
                        }
                    } else {
                        uint32_t   member_index        = 0;
                        uint32_t   member_offset       = 0;
                        uint32_t   current_data_offset = 0;
                        const auto child_el            = inline_array_el->FirstChildElement();
                        for (auto child = child_el; child != nullptr; child = child->NextSiblingElement()) {
                            uint32_t member_payload_offset = (payload_offset + member_offset);
                            uint32_t member_data_offset = (data_offset + (size * member_index)) + current_data_offset;

                            current_data_offset +=
                                XmlReadInstance(adf, child, subtype, payload, member_payload_offset, data_offset);

                            member_offset += subtype->m_Size;
                            ++member_index;
                        }
                    }

                    // NOTE: inline_array is... inline, don't need to adjust the payload offset now.
                    return 0;
                }

                break;
            }

            case EAdfType::String: {
                auto value = el->GetText();

                const uint32_t next_offset = -1;
                payload_offset             = jc::ALIGN_TO_BOUNDARY(payload_offset, type->m_Align);

                m_RefsToFix.push_back(payload_offset);

                // if we haven't yet wrote this string, write it now.
                const auto it = m_StringRefs.find(value);
                if (it == m_StringRefs.end()) {
                    // write the string into the data offset
                    std::memcpy((char*)payload + data_offset, value, strlen(value));

                    // write the reference to the data
                    std::memcpy((char*)payload + payload_offset, &data_offset, sizeof(data_offset));
                    std::memcpy((char*)payload + (payload_offset + 4), &next_offset, sizeof(next_offset));

                    m_StringRefs[value] = data_offset;

                    // return the aligned string length
                    return jc::ALIGN_TO_BOUNDARY(strlen(value) + 1, type->m_Align);
                }

                // get the first reference to the string, and write the offset
                const auto refed_offset = (*it).second;

                // write the reference to the data
                std::memcpy((char*)payload + payload_offset, &refed_offset, sizeof(refed_offset));
                std::memcpy((char*)payload + (payload_offset + 4), &next_offset, sizeof(next_offset));

                // NOTE: no return as we didn't write anything to the data offset
                break;
            }

            case EAdfType::Enumeration: {
                // find the enum member from the name index
                const auto name_index  = adf->GetStringIndex(el->GetText(), true);
                Enum*      enum_member = nullptr;
                for (uint32_t i = 0; i < type->m_MemberCount; ++i) {
                    const auto current_enum = (Enum*)((char*)type + sizeof(Type) + (sizeof(Enum) * i));
                    if (current_enum->m_Name == name_index) {
                        enum_member = current_enum;
                        break;
                    }
                }

                // TODO: if the member isn't found at this point, we're in big trouble
                // we should get out of the import and throw an error.
                assert(enum_member != nullptr);

                // write the enum index value
                std::memcpy((char*)payload + payload_offset, &enum_member->m_Value, sizeof(enum_member->m_Value));

                // NOTE: no return as we didn't write anything to the data offset
                break;
            }

            case EAdfType::StringHash: {
                auto stringhash_el = el->FirstChildElement("stringhash");
                if (!strcmp(el->Name(), "stringhash")) {
                    stringhash_el = el;
                }

                if (stringhash_el) {
                    const auto string = stringhash_el->GetText();
                    const auto hash   = hashlittle(string);
                    std::memcpy((char*)payload + payload_offset, &hash, sizeof(hash));
                }

                // NOTE: no return as we didn't write anything to the data offset
                break;
            }
        }

        return 0;
    }

    void Import(const std::filesystem::path& filename, ImportFinishedCallback callback) override final
    {
        using namespace jc::AvalancheDataFormat;

        SPDLOG_INFO("Importing ADF \"{}\"", filename.string());

        if (!std::filesystem::exists(filename)) {
            return callback(false, filename, 0);
        }

        tinyxml2::XMLDocument doc;
        doc.LoadFile(filename.string().c_str());

        const auto root = doc.FirstChildElement("adf");
        if (!root) {
            SPDLOG_ERROR("Couldn't find the root adf element.");
            return callback(false, filename, 0);
        }

        const auto extension = root->Attribute("extension");
        const auto version   = root->UnsignedAttribute("version");
        const auto flags     = root->UnsignedAttribute("flags");

        const auto  library_attr = root->Attribute("library");
        std::string library      = library_attr ? library_attr : "";
        util::replace(library, ", ", "\n");

        if (version != 4) {
            SPDLOG_ERROR("Unexpected adf version.");
            return callback(false, filename, 0);
        }

        // create adf
        // NOTE: this will load type libraries for us
        std::filesystem::path adf_filename = filename.stem().replace_extension(extension);
        const auto            adf          = AvalancheDataFormat::make(adf_filename, FileBuffer{});

        // load instances
        std::vector<Instance> instances;
        for (auto child = root->FirstChildElement("instance"); child != nullptr;
             child      = child->NextSiblingElement("instance")) {
            // add the instance name to the strings pool
            const auto instance_name = child->Attribute("name");
            VectorPushIfNotExists(adf->m_Strings, instance_name);
            VectorPushIfNotExists(adf->m_InternalStrings, instance_name);

            // load the instance
            Instance instance;
            instance.m_NameHash      = child->UnsignedAttribute("namehash");
            instance.m_TypeHash      = child->UnsignedAttribute("typehash");
            instance.m_PayloadOffset = child->UnsignedAttribute("offset");
            instance.m_PayloadSize   = child->UnsignedAttribute("size");
            instance.m_Name          = adf->GetStringIndex(instance_name, true);
            instances.emplace_back(std::move(instance));

            // load string hashes early
            // NOTE: we need to do this so we can calculate the expected file size before we start parsing the types
            PrefetchStringHashes(adf.get(), child);
        }

        // right now we only support single instances.
        if (instances.size() > 1) {
#ifdef DEBUG
            __debugbreak();
#endif
            SPDLOG_ERROR("ADF has multiple instances.");
            return callback(false, filename, {});
        }

        // load types
        uint32_t           types_count      = 0;
        uint32_t           total_types_size = 0;
        std::vector<Type*> types;
        const auto         types_element = root->FirstChildElement("types");
        if (types_element) {
            for (auto child = types_element->FirstChildElement("type"); child != nullptr;
                 child      = child->NextSiblingElement("type")) {
                // add the type name to the strings pool
                const auto type_name = child->Attribute("name");
                VectorPushIfNotExists(adf->m_Strings, type_name);
                VectorPushIfNotExists(adf->m_InternalStrings, type_name);

                // count members
                uint32_t member_count = 0;
                for (auto member = child->FirstChild(); member; member = member->NextSibling()) {
                    ++member_count;
                }

                const auto type    = (EAdfType)child->IntAttribute("type");
                const auto is_enum = (type == EAdfType::Enumeration);

                // allocate space for the type and members
                const auto type_size = sizeof(Type) + (member_count * (is_enum ? sizeof(Enum) : sizeof(Member)));
                const auto mem       = std::malloc(type_size);
                total_types_size += type_size;

                // init the type
                auto cast_type                   = (Type*)mem;
                cast_type->m_AdfType             = type;
                cast_type->m_Size                = child->UnsignedAttribute("size");
                cast_type->m_Align               = child->UnsignedAttribute("alignment");
                cast_type->m_TypeHash            = child->UnsignedAttribute("typehash");
                cast_type->m_Name                = adf->GetStringIndex(type_name, true);
                cast_type->m_Flags               = (uint16_t)child->UnsignedAttribute("flags");
                cast_type->m_ScalarType          = (ScalarType)child->IntAttribute("scalartype");
                cast_type->m_SubTypeHash         = child->UnsignedAttribute("subtypehash");
                cast_type->m_ArraySizeOrBitCount = child->UnsignedAttribute("arraysizeorbitcount");
                cast_type->m_MemberCount         = member_count;

                // read members
                uint32_t current_child_index = 0;
                for (auto member = child->FirstChildElement("member"); member != nullptr;
                     member      = member->NextSiblingElement("member")) {
                    // add the type member name to the strings pool
                    const auto member_name = member->Attribute("name");
                    VectorPushIfNotExists(adf->m_Strings, member_name);
                    VectorPushIfNotExists(adf->m_InternalStrings, member_name);

                    if (is_enum) {
                        Enum adf_enum;
                        adf_enum.m_Name  = adf->GetStringIndex(member_name, true);
                        adf_enum.m_Value = current_child_index;

                        // copy the member to the type block
                        void* ptr = ((char*)mem + sizeof(Type) + (sizeof(Enum) * current_child_index));
                        std::memcpy(ptr, &adf_enum, sizeof(adf_enum));
                    } else {
                        Member adf_member;
                        adf_member.m_Name         = adf->GetStringIndex(member_name, true);
                        adf_member.m_TypeHash     = member->UnsignedAttribute("typehash");
                        adf_member.m_Align        = member->UnsignedAttribute("alignment");
                        adf_member.m_Offset       = 0xDEAD; // NOTE: generated later
                        adf_member.m_BitOffset    = member->UnsignedAttribute("bitoffset");
                        adf_member.m_Flags        = member->UnsignedAttribute("flags");
                        adf_member.m_DefaultValue = member->Unsigned64Attribute("default");

                        // copy the member to the type block
                        void* ptr = ((char*)mem + sizeof(Type) + (sizeof(Member) * current_child_index));
                        std::memcpy(ptr, &adf_member, sizeof(adf_member));
                    }

                    ++current_child_index;
                }

                adf->m_Types.emplace_back((Type*)mem);
                types.emplace_back((Type*)mem);
                ++types_count;
            }
        }

        // calculate string hashes lengths
        const auto string_hashes_lengths =
            std::accumulate(adf->m_StringHashes.begin(), adf->m_StringHashes.end(), 0,
                            [](int32_t accumulator, const std::pair<uint32_t, std::string>& item) {
                                return accumulator + (item.second.length() + 1);
                            });

        // calculate string lengths
        const auto string_lengths = std::accumulate(
            adf->m_InternalStrings.begin(), adf->m_InternalStrings.end(), 0,
            [](int32_t accumulator, const std::string& str) { return accumulator + (str.length() + 1); });

        // calculate the first instance write offset
        const auto first_instance = instances.front();
        const auto first_instance_offset =
            jc::ALIGN_TO_BOUNDARY(first_instance.m_PayloadOffset + first_instance.m_PayloadSize
                                      + ((flags & EHeaderFlags::RELATIVE_OFFSETS_EXISTS) ? sizeof(uint32_t) : 0),
                                  8);

        //
        const auto total_string_hashes      = static_cast<uint32_t>(adf->m_StringHashes.size());
        const auto total_string_hashes_size = ((sizeof(uint64_t) * total_string_hashes) + string_hashes_lengths);

        // TODO: support multiple instances.

        // figure out how much data we need
        const auto     total_strings = static_cast<uint32_t>(adf->m_InternalStrings.size());
        const uint32_t file_size     = first_instance_offset + (sizeof(Instance) * instances.size()) + total_types_size
                                   + total_string_hashes_size + ((sizeof(uint8_t) * total_strings) + string_lengths);

        //
        adf->m_Buffer.resize(file_size);

        // generate the header
        Header header{};
        header.m_Magic                 = 0x41444620;
        header.m_Version               = 4;
        header.m_InstanceCount         = instances.size();
        header.m_FirstInstanceOffset   = first_instance_offset;
        header.m_TypeCount             = types_count;
        header.m_FirstTypeOffset       = (header.m_FirstInstanceOffset + (sizeof(Instance) * header.m_InstanceCount));
        header.m_StringHashCount       = static_cast<uint32_t>(adf->m_StringHashes.size());
        header.m_FirstStringHashOffset = (header.m_FirstTypeOffset + total_types_size);
        header.m_StringCount           = static_cast<uint32_t>(adf->m_InternalStrings.size());
        header.m_FirstStringDataOffset = (header.m_FirstStringHashOffset + total_string_hashes_size);
        header.m_FileSize              = file_size;
        header.unknown                 = 0;
        header.m_Flags                 = flags;
        header.m_IncludedLibraries     = 0;

        // reset the first type offset.
        // NOTE: we set the first type offset so then the string data offset can use the correct offset
        if (header.m_TypeCount == 0) {
            header.m_FirstTypeOffset = 0;
        }

        // same as above..
        if (header.m_StringHashCount == 0) {
            header.m_FirstStringHashOffset = 0;
        }

        // write the header
        std::memcpy(adf->m_Buffer.data(), &header, sizeof(header));
        std::memcpy(adf->m_Buffer.data() + offsetof(Header, m_Description), library.data(), library.length());

        // read type members
        for (auto child = root->FirstChildElement("instance"); child != nullptr;
             child      = child->NextSiblingElement("instance")) {
            const auto namehash = child->UnsignedAttribute("namehash");
            const auto it       = std::find_if(instances.begin(), instances.end(),
                                         [namehash](const Instance& item) { return item.m_NameHash == namehash; });

            if (it == instances.end()) {
                break;
            }

            const auto type = adf->FindType((*it).m_TypeHash);
            if (type) {
                XmlReadInstance(adf.get(), child, type, (const char*)&adf->m_Buffer[(*it).m_PayloadOffset], 0,
                                type->m_Size);

                // fix references
                // TODO: only if header flag 1 is set.
                // TODO: this won't work for ADFs with multiple instances - fix it.
                std::sort(m_RefsToFix.begin(), m_RefsToFix.end());
                for (uint32_t i = 0; i < m_RefsToFix.size(); ++i) {
                    const uint32_t offset      = m_RefsToFix[i];
                    uint32_t       next_offset = 0;

                    if (i != (m_RefsToFix.size() - 1)) {
                        next_offset = (m_RefsToFix[i + 1] - offset);
                    }

                    // update the current reference
                    // NOTE: +4 to skip the reference offset
                    *(uint32_t*)&adf->m_Buffer[(*it).m_PayloadOffset + (offset + 4)] = next_offset;
                }
            }

            // TODO: figure out what this is
            if (flags & EHeaderFlags::RELATIVE_OFFSETS_EXISTS) {
                const auto unknown = child->UnsignedAttribute("unknown");
                *(uint32_t*)&adf->m_Buffer[(*it).m_PayloadOffset + (*it).m_PayloadSize] = unknown;
            }
        }

        // write the instances
        std::memcpy(adf->m_Buffer.data() + header.m_FirstInstanceOffset, instances.data(),
                    (sizeof(Instance) * instances.size()));

        // write the types
        uint32_t cur_type_offset = 0;
        for (const auto type : types) {
            const uint32_t size = type->Size();
            std::memcpy(adf->m_Buffer.data() + header.m_FirstTypeOffset + cur_type_offset, (char*)type, size);
            cur_type_offset += size;
        }

        // write the string hashes
        uint32_t cur_string_hash_offset = 0;
        for (auto it = adf->m_StringHashes.begin(); it != adf->m_StringHashes.end(); ++it) {
            const auto  hash   = (*it).first;
            const auto& string = (*it).second;

            // write the string
            std::memcpy(adf->m_Buffer.data() + header.m_FirstStringHashOffset + cur_string_hash_offset, string.data(),
                        string.size());
            cur_string_hash_offset += (string.length() + 1);

            // write the string hash
            // NOTE: format uses uint64_t NOT uint32_t
            std::memcpy(adf->m_Buffer.data() + header.m_FirstStringHashOffset + cur_string_hash_offset, &hash,
                        sizeof(hash));
            cur_string_hash_offset += sizeof(uint64_t);
        }

        // write the strings
        uint32_t total_lengths     = (sizeof(uint8_t) * total_strings);
        uint32_t cur_string_offset = 0;
        for (uint32_t i = 0; i < adf->m_InternalStrings.size(); ++i) {
            const auto& string = adf->m_InternalStrings[i];
            uint8_t     length = string.length();

            // write the string length
            std::memcpy(adf->m_Buffer.data() + header.m_FirstStringDataOffset + (sizeof(uint8_t) * i), &length,
                        sizeof(length));

            // write the string
            std::memcpy(adf->m_Buffer.data() + header.m_FirstStringDataOffset + total_lengths + cur_string_offset,
                        string.data(), length + 1);

            cur_string_offset += length + 1;
        }

        //
        m_StringRefs.clear();
        m_RefsToFix.clear();

        auto new_filename = filename.parent_path() / adf_filename;
        return callback(true, new_filename, adf->m_Buffer);
    }

    void XmlWriteInstance(::AvalancheDataFormat* adf, tinyxml2::XMLPrinter& printer,
                          const jc::AvalancheDataFormat::Header* header, const jc::AvalancheDataFormat::Type* type,
                          const char* payload, uint32_t offset = 0)
    {
        using namespace jc::AvalancheDataFormat;

        switch (type->m_AdfType) {
            case EAdfType::Primitive: {
                // clang-format off
                switch (type->m_ScalarType) {
                    case ScalarType::Signed:
                        switch (type->m_Size) {
                            case sizeof(int8_t):	printer.PushText(*(int8_t*)&payload[offset]); break;
                            case sizeof(int16_t):	printer.PushText(*(int16_t*)&payload[offset]); break;
                            case sizeof(int32_t):	printer.PushText(*(int32_t*)&payload[offset]); break;
                            case sizeof(int64_t):	printer.PushText(*(int64_t*)&payload[offset]); break;
                        }
                        break;
                    case ScalarType::Unsigned:
                        switch (type->m_Size) {
                            case sizeof(uint8_t):	printer.PushText(*(uint8_t*)&payload[offset]); break;
                            case sizeof(uint16_t):	printer.PushText(*(uint16_t*)&payload[offset]); break;
                            case sizeof(uint32_t):	printer.PushText(*(uint32_t*)&payload[offset]); break;
                            case sizeof(uint64_t):	printer.PushText(*(uint64_t*)&payload[offset]); break;
                        }
                        break;
                    case ScalarType::Float:
                        switch (type->m_Size) {
                            case sizeof(float):	PushHigherPrecisionFloat(printer, *(float*)&payload[offset]); break;
                            case sizeof(double):	printer.PushText(*(double*)&payload[offset]); break;
                        }
                        break;
                }
                // clang-format on

                break;
            }

            case EAdfType::Structure: {
                printer.OpenElement("struct");
                printer.PushAttribute("type", adf->m_Strings[type->m_Name].c_str());

                for (uint32_t i = 0; i < type->m_MemberCount; ++i) {
                    const Member& member      = type->Member(i);
                    const Type*   member_type = adf->FindType(member.m_TypeHash);

                    assert(member_type);

                    printer.OpenElement("member");
                    printer.PushAttribute("name", adf->m_Strings[member.m_Name].c_str());

                    if (member_type->m_AdfType == EAdfType::BitField) {
                        const auto bit_count = member_type->m_ArraySizeOrBitCount;

                        auto data = *(uint64_t*)&payload[(offset + (member.m_Offset & 0xFFFFFF))];

                        const int64_t v26 = (1 << bit_count);
                        int64_t       v27 = (v26 - 1) & (data >> member.m_BitOffset);

                        if (member_type->m_ScalarType == ScalarType::Signed
                            && _bittest64((long long*)&v27, (bit_count - 1))) {
                            v27 -= v26;
                        }

                        // TODO: signed values seem wrong? showing -4 instead of 4
                        printer.PushText(v27);
                    } else {
                        XmlWriteInstance(adf, printer, header, member_type, payload,
                                         (offset + (member.m_Offset & 0xFFFFFF)));
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

                    if (data_offset) {
                        XmlWriteInstance(adf, printer, header, deferred_type, payload, data_offset);
                    }

                    printer.CloseElement();
                }

                break;
            }

            case EAdfType::Array: {
                printer.OpenElement("array");

                const auto read_offset = *(uint32_t*)&payload[offset];
                const auto count       = *(uint32_t*)&payload[offset + 8];

                printer.PushAttribute("count", count);

                // TODO: better support for binary data

                const auto subtype = adf->FindType(type->m_SubTypeHash);
                for (uint32_t i = 0; i < count; ++i) {
                    XmlWriteInstance(adf, printer, header, subtype, payload, (read_offset + (subtype->m_Size * i)));

                    // if it's not the last element, add a space
                    if (subtype->m_AdfType == EAdfType::Primitive && (i != (count - 1))) {
                        printer.PushText(" ");
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
                const uint32_t string_offset = *(uint32_t*)&payload[offset];
                const char*    value         = (const char*)&payload[string_offset];
                printer.PushText(value);
                break;
            }

            case EAdfType::Enumeration: {
                const uint32_t index       = *(uint32_t*)&payload[offset];
                const Enum&    enum_member = type->Enum(index);

                printer.PushText(adf->m_Strings[enum_member.m_Name].c_str());
                break;
            }

            case EAdfType::StringHash: {
                printer.OpenElement("stringhash");

                const uint32_t string_hash = *(uint32_t*)&payload[offset];
                const char*    string      = adf->HashLookup(string_hash);

                printer.PushText(string);
                printer.CloseElement();
                break;
            }
        }
    }

    bool DoExport(const std::filesystem::path& path, ::AvalancheDataFormat* adf)
    {
        using namespace jc::AvalancheDataFormat;

        const auto& buffer = adf->GetBuffer();

        Header      header_out{};
        bool        byte_swap_out;
        const char* description_out = "";
        if (!adf->ParseHeader((Header*)buffer.data(), buffer.size(), &header_out, &byte_swap_out, &description_out)) {
            return false;
        }

        tinyxml2::XMLPrinter printer;
        printer.PushHeader(false, true);
        printer.PushComment(" File generated by " VER_PRODUCTNAME_STR " v" VER_PRODUCT_VERSION_STR " ");
        printer.PushComment(" https://github.com/aaronkirkham/jc-model-renderer ");

        printer.OpenElement("adf");
        XmlPushAttribute("extension", adf->GetFilename().extension().string().c_str());
        XmlPushAttribute("version", header_out.m_Version);
        XmlPushAttribute("flags", header_out.m_Flags);

        if (strlen(description_out) > 0) {
            std::string description(description_out);
            util::replace(description, "\n", ", ");

            printer.PushAttribute("library", description.c_str());
        }

        // instances
        auto instance_buffer = &buffer[header_out.m_FirstInstanceOffset];
        for (uint32_t i = 0; i < header_out.m_InstanceCount; ++i) {
            const Instance* instance = (Instance*)instance_buffer;

            printer.OpenElement("instance");
            XmlPushAttribute("name", adf->m_Strings[instance->m_Name].c_str());
            XmlPushAttribute("namehash", instance->m_NameHash);
            XmlPushAttribute("typehash", instance->m_TypeHash);
            XmlPushAttribute("offset", instance->m_PayloadOffset);
            XmlPushAttribute("size", instance->m_PayloadSize);

            // TODO: don't write this when we know what it is actually for
            if (header_out.m_Flags & EHeaderFlags::RELATIVE_OFFSETS_EXISTS) {
                const auto unknown = *(uint32_t*)&adf->m_Buffer[instance->m_PayloadOffset + instance->m_PayloadSize];
                XmlPushAttribute("unknown", unknown);
            }

            // write the type members
            const auto type = adf->FindType(instance->m_TypeHash);
            if (type) {
                XmlWriteInstance(adf, printer, &header_out, type, (const char*)&buffer[instance->m_PayloadOffset], 0);
            }

            printer.CloseElement();

            instance_buffer += sizeof(Instance);
        }

        // types
        if (header_out.m_TypeCount > 0) {
            printer.PushComment(" You should really leave these alone! ");
            printer.OpenElement("types");

            auto types_buffer = &adf->m_Buffer[header_out.m_FirstTypeOffset];
            for (uint32_t i = 0; i < header_out.m_TypeCount; ++i) {
                const Type* type = (Type*)types_buffer;

                printer.OpenElement("type");
                XmlPushAttribute("name", adf->m_Strings[type->m_Name].c_str());
                XmlPushAttribute("type", static_cast<uint32_t>(type->m_AdfType));
                XmlPushAttribute("size", type->m_Size);
                XmlPushAttribute("alignment", type->m_Align);
                XmlPushAttribute("typehash", type->m_TypeHash);
                XmlPushAttribute("flags", type->m_Flags);
                XmlPushAttribute("scalartype", static_cast<uint32_t>(type->m_ScalarType));
                XmlPushAttribute("subtypehash", type->m_SubTypeHash);
                XmlPushAttribute("arraysizeorbitcount", type->m_ArraySizeOrBitCount);

                const auto is_enum = (type->m_AdfType == EAdfType::Enumeration);
                for (uint32_t y = 0; y < type->m_MemberCount; ++y) {
                    printer.OpenElement("member");

                    if (is_enum) {
                        const Enum& enum_member = type->Enum(y);
                        XmlPushAttribute("name", adf->m_Strings[enum_member.m_Name].c_str());
                    } else {
                        const Member& member = type->m_Members[y];
                        XmlPushAttribute("name", adf->m_Strings[member.m_Name].c_str());
                        XmlPushAttribute("typehash", member.m_TypeHash);
                        XmlPushAttribute("alignment", member.m_Align);
                        XmlPushAttribute("bitoffset", member.m_BitOffset);
                        XmlPushAttribute("flags", member.m_Flags);
                        XmlPushAttribute("default", member.m_DefaultValue);
                    }

                    printer.CloseElement();
                }

                printer.CloseElement();

                // next element
                types_buffer += type->Size();
            }

            printer.CloseElement();
        }

        printer.CloseElement();

        // write
        std::ofstream out_stream(path);
        out_stream << printer.CStr();
        out_stream.close();

        return true;
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

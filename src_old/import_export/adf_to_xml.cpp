#pragma once

#include <AvaFormatLib.h>
#include <spdlog/spdlog.h>
#include <tinyxml2.h>

#include <algorithm>
#include <numeric>

#include "adf_to_xml.h"
#include "util.h"
#include "version.h"

#include "game/file_loader.h"
#include "game/formats/avalanche_archive.h"

namespace ImportExport
{
#define VectorPushIfNotExists(container, value)                                                                        \
    if (std::find(container.begin(), container.end(), value) == container.end()) {                                     \
        container.push_back(value);                                                                                    \
    }

#define XmlForeachChild(parent, name)                                                                                  \
    for (auto child = parent->FirstChildElement(name); child != nullptr; child = child->NextSiblingElement(name))

static std::vector<std::string>        Strings;
static std::map<uint32_t, std::string> StringHashes;
static std::vector<uint32_t>           RefsToFix;
static std::map<std::string, uint32_t> StringRefs;

static void PrefetchStringHashes(tinyxml2::XMLElement *el)
{
    if (!strcmp(el->Name(), "stringhash")) {
        const auto text = el->GetText();
        if (text && strlen(text) > 0) {
            const auto hash    = ava::hashlittle(text);
            StringHashes[hash] = text;
        }
    }

    for (auto child = el->FirstChildElement(); child != nullptr; child = child->NextSiblingElement()) {
        PrefetchStringHashes(child);
    }
}

static void PushPrimitiveTypeAttribute(tinyxml2::XMLPrinter &printer, uint32_t type_hash)
{
    // clang-format off
    switch (type_hash) {
        case 0xca2821d:  printer.PushAttribute("type", "uint8");  break;
        case 0x580d0a62: printer.PushAttribute("type", "int8");   break;
        case 0x86d152bd: printer.PushAttribute("type", "uint16"); break;
        case 0xd13fcf93: printer.PushAttribute("type", "int16");  break;
        case 0x75e4e4f:  printer.PushAttribute("type", "uint32"); break;
        case 0x192fe633: printer.PushAttribute("type", "int32");  break;
        case 0xa139e01f: printer.PushAttribute("type", "uint64"); break;
        case 0xaf41354f: printer.PushAttribute("type", "int64");  break;
        case 0x7515a207: printer.PushAttribute("type", "float");  break;
        case 0xc609f663: printer.PushAttribute("type", "double"); break;
        case 0x8955583e: printer.PushAttribute("type", "string"); break;
    }
    // clang-format on
}

template <typename T> static inline void WriteScalar(T value, const char *data, uint32_t offset)
{
    std::memcpy((char *)data + offset, &value, sizeof(T));
}

static void WriteScalarFromString(const ava::AvalancheDataFormat::AdfType *type, const char *data, uint32_t offset,
                                  const std::string &str)
{
    using namespace ava::AvalancheDataFormat;

    // clang-format off
    switch (type->m_ScalarType) {
        case ADF_SCALARTYPE_SIGNED:
            switch (type->m_Size) {
                case sizeof(int8_t):   WriteScalar<int8_t>(std::stoi(str), data, offset); break;
                case sizeof(int16_t):  WriteScalar<int16_t>(std::stoi(str), data, offset); break;
                case sizeof(int32_t):  WriteScalar<int32_t>(std::stol(str), data, offset); break;
                case sizeof(int64_t):  WriteScalar<int64_t>(std::stoll(str), data, offset); break;
            }
            break;
        case ADF_SCALARTYPE_UNSIGNED:
            switch (type->m_Size) {
                case sizeof(uint8_t):  WriteScalar<uint8_t>(std::stoi(str), data, offset); break;
                case sizeof(uint16_t): WriteScalar<uint16_t>(std::stoi(str), data, offset); break;
                case sizeof(uint32_t): WriteScalar<uint32_t>(std::stoul(str), data, offset); break;
                case sizeof(uint64_t): WriteScalar<uint64_t>(std::stoull(str), data, offset); break;
            }
            break;
        case ADF_SCALARTYPE_FLOAT:
            switch (type->m_Size) {
                case sizeof(float):    WriteScalar(std::stof(str), data, offset); break;
                case sizeof(double):   WriteScalar(std::stod(str), data, offset); break;
            }
            break;
    }
    // clang-format on
}

// @NOTE: Only return a positive value if we write to the DATA offset (NOT THE PAYLOAD OFFSET)
static uint32_t ReadInstance(tinyxml2::XMLElement *el, ava::AvalancheDataFormat::ADF *adf,
                             const ava::AvalancheDataFormat::AdfHeader *header,
                             const ava::AvalancheDataFormat::AdfType *type, const char *data, uint32_t offset,
                             uint32_t data_offset)
{
    using namespace ava::AvalancheDataFormat;

    switch (type->m_Type) {
        case ADF_TYPE_SCALAR: {
            // clang-format off
            switch (type->m_ScalarType) {
                case ADF_SCALARTYPE_SIGNED:
                    switch (type->m_Size) {
                        case sizeof(int8_t):   WriteScalar<int8_t>(el->IntText(), data, offset); break;
                        case sizeof(int16_t):  WriteScalar<int16_t>(el->IntText(), data, offset); break;
                        case sizeof(int32_t):  WriteScalar<int32_t>(el->IntText(), data, offset); break;
                        case sizeof(int64_t):  WriteScalar<int64_t>(el->Int64Text(), data, offset); break;
                    }
                    break;
                case ADF_SCALARTYPE_UNSIGNED:
                    switch (type->m_Size) {
                        case sizeof(uint8_t):  WriteScalar<uint8_t>(el->UnsignedText(), data, offset); break;
                        case sizeof(uint16_t): WriteScalar<uint16_t>(el->UnsignedText(), data, offset); break;
                        case sizeof(uint32_t): WriteScalar<uint32_t>(el->UnsignedText(), data, offset); break;
                        case sizeof(uint64_t): WriteScalar<uint64_t>(el->Unsigned64Text(), data, offset); break;
                    }
                    break;
                case ADF_SCALARTYPE_FLOAT:
                    switch (type->m_Size) {
                        case sizeof(float):    WriteScalar(el->FloatText(), data, offset); break;
                        case sizeof(double):   WriteScalar(el->DoubleText(), data, offset); break;
                    }
                    break;
            }
            // clang-format on
            break;
        }

        case ADF_TYPE_STRUCT: {
            auto struct_el = el->FirstChildElement("struct");
            if (!strcmp(el->Name(), "struct")) {
                struct_el = el;
            }

            if (!struct_el) {
                break;
            }

            uint32_t   member_index        = 0;
            uint32_t   member_offset       = 0;
            uint32_t   current_data_offset = 0;
            const auto member_el           = struct_el->FirstChildElement("member");
            for (auto child = member_el; child != nullptr; child = child->NextSiblingElement("member")) {
                // get the current member from the child index
                const AdfMember &current_member = type->m_Members[member_index];
                const AdfType *  member_type    = adf->FindType(current_member.m_TypeHash);

                // get the offset where to write the next element
                uint32_t       real_payload_offset = (offset + member_offset);
                const uint32_t alignment = ava::math::align_distance(real_payload_offset, member_type->m_Align);
                real_payload_offset += alignment;

                // get the offset where we want to write the actual data
                // NOTE: if things go bad, check without the alignment
                const uint32_t real_data_offset =
                    ava::math::align(data_offset + current_data_offset, member_type->m_Align);

                SPDLOG_INFO("Struct {} @ {:x}", Strings[type->m_Name], real_data_offset);

                if (type->m_Name == 1) {
                    assert(real_data_offset != 0x2408);
                }

                if (member_type->m_Type == ADF_TYPE_BITFIELD) {
                    // get the current bit value
                    uint64_t value = 0;
                    if (member_type->m_ScalarType == ADF_SCALARTYPE_SIGNED) {
                        value = child->Int64Text();
                    } else if (member_type->m_ScalarType == ADF_SCALARTYPE_UNSIGNED) {
                        value = child->Unsigned64Text();
                    }

                    // TODO: we should generate bit_offset our selfs.
                    auto v7            = value & ((1 << member_type->m_BitCount) - 1);
                    auto current_value = *(uint32_t *)&data[real_payload_offset];
                    *(uint32_t *)&data[real_payload_offset] |= (v7 << current_member.m_BitOffset);
                } else {
                    // read child instance
                    current_data_offset +=
                        ReadInstance(child, adf, header, member_type, data, real_payload_offset, real_data_offset);

                    if (current_data_offset == 0) {
                        __debugbreak();
                    }

                    // next member offset
                    member_offset += (member_type->m_Size + alignment);
                }

                ++member_index;
            }

            return current_data_offset;
        }

        case ADF_TYPE_POINTER:
        case ADF_TYPE_DEFERRED: {
            auto pointer_el = el->FirstChildElement("pointer");
            if (!strcmp(el->Name(), "pointer")) {
                pointer_el = el;
            }

            if (!pointer_el) {
                break;
            }

            const uint32_t type_hash     = pointer_el->UnsignedAttribute("type");
            const AdfType *deferred_type = adf->FindType(type_hash);

            if (!deferred_type || !pointer_el->FirstChildElement()) {
                break;
            }

            // write a reference to the data
            static const uint32_t next_offset = -1;
            std::memcpy((char *)data + offset, &data_offset, sizeof(data_offset));
            std::memcpy((char *)data + offset + sizeof(data_offset), &next_offset, sizeof(next_offset));
            std::memcpy((char *)data + offset + sizeof(data_offset) + sizeof(next_offset), &type_hash,
                        sizeof(type_hash));

            RefsToFix.push_back(offset);

            // read the pointer instance
            const uint32_t deferred_data_offset = (data_offset + deferred_type->m_Size);
            const uint32_t size = ReadInstance(pointer_el->FirstChildElement(), adf, header, deferred_type, data,
                                               data_offset, deferred_data_offset);
            return (deferred_data_offset - (data_offset - size));
        }

        case ADF_TYPE_ARRAY: {
            const auto array_el = el->FirstChildElement("array");
            if (!array_el) {
                break;
            }

            // @TODO: dont read/write array count. we should be able to figure this out from the type
            const uint32_t count = array_el->UnsignedAttribute("count");

            // find the array member sub type
            const AdfType *sub_type = adf->FindType(type->m_SubTypeHash);
            if (!sub_type) {
                break;
            }

            // write zeros if we have no members!
            if (count == 0) {
                static uint32_t zero = 0;
                std::memcpy((char *)data + offset, &zero, sizeof(zero));
                std::memcpy((char *)data + offset + sizeof(zero), &zero, sizeof(zero));
                std::memcpy((char *)data + offset + sizeof(zero) + sizeof(zero), &zero, sizeof(zero));

                bool has_32bit_inline_arrays = ~LOBYTE(header->m_Flags) & E_ADF_HEADER_FLAG_RELATIVE_OFFSETS_EXISTS;
                return (has_32bit_inline_arrays ? sub_type->m_Align : 0);
            }

            uint32_t written = 0;
            if (sub_type->m_Type == ADF_TYPE_SCALAR) {
                const auto &str_data = util::split(array_el->GetText(), " ");
                assert(str_data.size() == count);
                for (uint32_t i = 0; i < count; ++i) {
                    WriteScalarFromString(sub_type, data, (data_offset + (sub_type->m_Size * i)), str_data[i]);
                }

                written = ava::math::align((sub_type->m_Size * count), type->m_Align);
            } else {
                // read array members
                uint32_t   member_offset       = 0;
                uint32_t   current_data_offset = 0;
                const auto array_member_el     = array_el->FirstChildElement();

                SPDLOG_INFO("Array {}", Strings[type->m_Name]);

                if (data_offset == 0x2110) {
                    __debugbreak();
                }

                for (auto child = array_member_el; child != nullptr; child = child->NextSiblingElement()) {
                    // calculate data
                    const uint32_t member_data_offset =
                        (data_offset + (sub_type->m_Size * count) + current_data_offset);

                    // write the instance
                    current_data_offset += ReadInstance(child, adf, header, sub_type, data,
                                                        (data_offset + member_offset), member_data_offset);

                    SPDLOG_INFO(" - {} Offset={:x}, DataOffset={:x}", Strings[sub_type->m_Name],
                                (data_offset + member_offset), member_data_offset);

                    // next
                    member_offset += sub_type->m_Size;
                }

                written = ava::math::align((sub_type->m_Size * count) + current_data_offset, type->m_Align);
            }

            // write a reference to the data
            static const uint32_t next_offset = -1;
            std::memcpy((char *)data + offset, &data_offset, sizeof(data_offset));
            std::memcpy((char *)data + offset + sizeof(data_offset), &next_offset, sizeof(next_offset));
            std::memcpy((char *)data + offset + sizeof(data_offset) + sizeof(next_offset), &count, sizeof(count));

            RefsToFix.push_back(offset);
            return written;
        }

        case ADF_TYPE_INLINE_ARRAY: {
            auto inline_array_el = el->FirstChildElement("inline_array");
            if (!strcmp(el->Name(), "inline_array")) {
                inline_array_el = el;
            }

            if (!inline_array_el) {
                break;
            }

            // find the array member sub type
            const AdfType *sub_type = adf->FindType(type->m_SubTypeHash);
            uint32_t       size     = sub_type->m_Size;
            if (sub_type->m_Type == ADF_TYPE_POINTER || sub_type->m_Type == ADF_TYPE_STRING) {
                size = 8;
            }

            if (sub_type->m_Type == ADF_TYPE_SCALAR) {
                const auto &str_data = util::split(inline_array_el->GetText(), " ");

                // @NOTE: inline array are fixed-size.
                // @TODO: this should be non-fatal. we can simply drop the overflowing elements and proceed.
                if (str_data.size() != type->m_ArraySize) {
                    throw std::runtime_error("inline_array element was not the size we expected! Size="
                                             + std::to_string(str_data.size())
                                             + ", Expected=" + std::to_string(type->m_Align));
                }

                // write array members
                for (uint32_t i = 0; i < type->m_ArraySize; ++i) {
                    WriteScalarFromString(sub_type, data, (offset + (size * i)), str_data[i]);
                }
            } else {
                uint32_t   member_index        = 0;
                uint32_t   current_data_offset = 0;
                const auto child_el            = inline_array_el->FirstChildElement();
                for (auto child = child_el; child != nullptr; child = child->NextSiblingElement()) {
                    ReadInstance(child, adf, header, sub_type, data, (offset + (size * member_index)), data_offset);
                    ++member_index;
                }
            }

            break;
        }

        case ADF_TYPE_STRING: {
            const char *el_value = el->GetText();
            std::string value    = el_value ? el_value : "";

            static const uint32_t next_offset = -1;
            RefsToFix.push_back(offset);

            // if we didn't already write this string, write it now!
            const auto it = StringRefs.find(value);
            if (it == StringRefs.end()) {
                // write the string to the data offset
                std::memcpy((char *)data + data_offset, value.data(), value.size());
                // StringRefs[value] = data_offset;

                // @TODO: look at this!
                //	      Disabled string refs for now..

                // write the reference in the payload offset
                std::memcpy((char *)data + offset, &data_offset, sizeof(data_offset));
                std::memcpy((char *)data + offset + sizeof(data_offset), &next_offset, sizeof(next_offset));
                return ava::math::align(((uint32_t)value.length() + 1), type->m_Align);
            }

            // write the offset to the reference instead!
            const uint32_t ref_data_offset = (*it).second;
            std::memcpy((char *)data + offset, &ref_data_offset, sizeof(ref_data_offset));
            std::memcpy((char *)data + offset + sizeof(ref_data_offset), &next_offset, sizeof(next_offset));
            break;
        }

        case ADF_TYPE_ENUM: {
            // @TODO - See WriteInstance
            const uint32_t value = el->UnsignedText();
            std::memcpy((char *)data + offset, &value, sizeof(value));
            break;
        }

        case ADF_TYPE_STRING_HASH: {
            auto stringhash_el = el->FirstChildElement("stringhash");
            if (!strcmp(el->Name(), "stringhash")) {
                stringhash_el = el;
            }

            if (!stringhash_el) {
                break;
            }

            const auto string = stringhash_el->GetText();
            if (string && strlen(string) > 0) {
                const uint32_t hash = ava::hashlittle(string);
                std::memcpy((char *)data + offset, &hash, sizeof(hash));

                SPDLOG_INFO("StringHash: {} ({:x})", string, hash);
            }

            break;
        }
    }

    return 0;
}

void ADF2XML::Import(const std::filesystem::path &filename, ImportFinishedCallback callback)
{
    using namespace ava::AvalancheDataFormat;

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
        SPDLOG_ERROR("Unsupported adf version.");
        return callback(false, filename, 0);
    }

    // create adf
    // NOTE: this will load type libraries for us
    std::filesystem::path adf_filename = filename.stem().replace_extension(extension);
    // const auto            adf          = AvalancheDataFormat::make(adf_filename, FileBuffer{});

    ADF2XMLHelper adf{};

    static auto StringIndex = [&](const char *value) -> uint64_t {
        return std::distance(Strings.begin(), std::find(Strings.begin(), Strings.end(), value));
    };

    // load instances
    std::vector<AdfInstance> instances;
    XmlForeachChild(root, "instance")
    {
        // add the instance name to the strings pool
        const auto instance_name = child->Attribute("name");
        VectorPushIfNotExists(Strings, instance_name);
        // VectorPushIfNotExists(adf->m_InternalStrings, instance_name);

        // load the instance
        AdfInstance instance;
        instance.m_NameHash      = ava::hashlittle(instance_name);
        instance.m_TypeHash      = child->UnsignedAttribute("typehash");
        instance.m_PayloadOffset = child->UnsignedAttribute("offset");
        instance.m_PayloadSize   = child->UnsignedAttribute("size");
        instance.m_Name          = StringIndex(instance_name);
        instances.emplace_back(std::move(instance));

        // load string hashes early
        // NOTE: we need to do this so we can calculate the expected file size before we start parsing the types
        PrefetchStringHashes(child);
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
    uint32_t               total_types_size = 0;
    std::vector<AdfType *> types;
    const auto             types_el = root->FirstChildElement("types");
    if (types_el) {
        XmlForeachChild(types_el, "type")
        {
            const char *name = child->Attribute("name");
            VectorPushIfNotExists(Strings, name);
            // VectorPushIfNotExists(m_InternalStrings, name);

            // count members
            uint32_t member_count = 0;
            for (auto member = child->FirstChild(); member; member = member->NextSibling()) {
                ++member_count;
            }

            const EAdfType adf_type = (EAdfType)child->IntAttribute("type");
            const bool     is_enum  = (adf_type == ADF_TYPE_ENUM);

            // allocate space for the type and members
            const size_t size = (sizeof(AdfType) + (member_count * (is_enum ? sizeof(AdfEnum) : sizeof(AdfMember))));
            auto         type = (AdfType *)std::malloc(size);
            if (!type) {
                SPDLOG_ERROR("Can't allocate enough space for type.");
                return callback(false, filename, {});
            }

            // track the total allocated size for the ADF header generation later.
            total_types_size += (uint32_t)size;

            // initialise the type
            type->m_Type        = adf_type;
            type->m_Size        = child->UnsignedAttribute("size");
            type->m_Align       = child->UnsignedAttribute("align");
            type->m_TypeHash    = child->UnsignedAttribute("typehash");
            type->m_Name        = StringIndex(name);
            type->m_Flags       = (uint16_t)child->UnsignedAttribute("flags");
            type->m_ScalarType  = (EAdfScalarType)child->UnsignedAttribute("scalartype");
            type->m_SubTypeHash = child->UnsignedAttribute("subtypehash");
            type->m_ArraySize   = child->UnsignedAttribute("count");
            type->m_MemberCount = member_count;

            // initialise type members
            uint32_t   member_index = 0;
            const auto member_el    = child->FirstChildElement("member");
            for (auto member = member_el; member != nullptr; member = member->NextSiblingElement("member")) {
                const char *name = member->Attribute("name");
                VectorPushIfNotExists(Strings, name);
                // VectorPushIfNotExists(m_InternalStrings, name);

                if (is_enum) {
                    AdfEnum &memb = type->Enum(member_index);
                    memb.m_Name   = StringIndex(name);
                    memb.m_Value  = member->IntAttribute("value");
                } else {
                    AdfMember &memb     = type->m_Members[member_index];
                    memb.m_Name         = StringIndex(name);
                    memb.m_TypeHash     = member->UnsignedAttribute("typehash");
                    memb.m_Align        = member->UnsignedAttribute("align");
                    memb.m_Offset       = member->UnsignedAttribute("offset");
                    memb.m_BitOffset    = member->UnsignedAttribute("bitoffset");
                    memb.m_Flags        = member->UnsignedAttribute("flags");
                    memb.m_DefaultValue = member->UnsignedAttribute("default");
                }

                ++member_index;
            }

            //
            adf.PushType(type);
            types.push_back(type);
        }
    }

    // calculate string hashes lengths
    const auto string_hashes_lengths =
        std::accumulate(StringHashes.begin(), StringHashes.end(), 0,
                        [](int32_t accumulator, const std::pair<uint32_t, std::string> &item) {
                            return accumulator + (item.second.length() + 1);
                        });

    // calculate string lengths
    const auto string_lengths =
        std::accumulate(Strings.begin(), Strings.end(), 0,
                        [](int32_t accumulator, const std::string &str) { return accumulator + (str.length() + 1); });

    // calculate the first instance write offset
    const auto first_instance = instances.front();
    const auto first_instance_offset =
        ava::math::align(first_instance.m_PayloadOffset + first_instance.m_PayloadSize
                             + ((flags & E_ADF_HEADER_FLAG_RELATIVE_OFFSETS_EXISTS) ? sizeof(uint32_t) : 0),
                         8);

    //
    const auto total_string_hashes      = static_cast<uint32_t>(StringHashes.size());
    const auto total_string_hashes_size = ((sizeof(uint64_t) * total_string_hashes) + string_hashes_lengths);

    // TODO: support multiple instances.

    // figure out how much data we need
    const auto     total_strings = static_cast<uint32_t>(Strings.size());
    const uint32_t file_size     = first_instance_offset + (sizeof(AdfInstance) * instances.size()) + total_types_size
                               + total_string_hashes_size + ((sizeof(uint8_t) * total_strings) + string_lengths);

    //
    std::vector<uint8_t> buffer(file_size);

    // generate the header
    AdfHeader header{};
    header.m_Magic                 = ADF_MAGIC;
    header.m_Version               = 4;
    header.m_InstanceCount         = instances.size();
    header.m_FirstInstanceOffset   = first_instance_offset;
    header.m_TypeCount             = static_cast<uint32_t>(types.size());
    header.m_FirstTypeOffset       = (header.m_FirstInstanceOffset + (sizeof(AdfInstance) * header.m_InstanceCount));
    header.m_StringHashCount       = static_cast<uint32_t>(StringHashes.size());
    header.m_FirstStringHashOffset = (header.m_FirstTypeOffset + total_types_size);
    header.m_StringCount           = static_cast<uint32_t>(Strings.size());
    header.m_FirstStringDataOffset = (header.m_FirstStringHashOffset + total_string_hashes_size);
    header.m_FileSize              = file_size;
    header.m_Flags                 = flags;

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
    std::memcpy(buffer.data(), &header, sizeof(header));
    std::memcpy(buffer.data() + offsetof(AdfHeader, m_Description), library.data(), library.length());

    // read type members
    for (auto child = root->FirstChildElement("instance"); child != nullptr;
         child      = child->NextSiblingElement("instance")) {
        const auto namehash = ava::hashlittle(child->Attribute("name"));
        const auto it       = std::find_if(instances.begin(), instances.end(),
                                     [namehash](const AdfInstance &item) { return item.m_NameHash == namehash; });

        if (it == instances.end()) {
            break;
        }

        const auto type = adf.FindType((*it).m_TypeHash);
        if (type) {
            ReadInstance(child, &adf, &header, type, (const char *)&buffer[(*it).m_PayloadOffset], 0, type->m_Size);

            // fix relative offsets
            if (header.m_Flags & E_ADF_HEADER_FLAG_RELATIVE_OFFSETS_EXISTS) {
                // TODO: this won't work for ADFs with multiple instances - fix it.
                std::sort(RefsToFix.begin(), RefsToFix.end());
                for (uint32_t i = 0; i < RefsToFix.size(); ++i) {
                    const uint32_t offset      = RefsToFix[i];
                    uint32_t       next_offset = 0;

                    if (i != (RefsToFix.size() - 1)) {
                        next_offset = (RefsToFix[i + 1] - offset);
                    }

                    // update the current reference
                    // NOTE: +4 to skip the reference offset
                    *(uint32_t *)&buffer[(*it).m_PayloadOffset + (offset + 4)] = next_offset;
                }
            }
        }

        // TODO: figure out what this is
        if (flags & E_ADF_HEADER_FLAG_RELATIVE_OFFSETS_EXISTS) {
            const auto unknown                                                = child->UnsignedAttribute("unknown");
            *(uint32_t *)&buffer[(*it).m_PayloadOffset + (*it).m_PayloadSize] = unknown;
        }
    }

    // write the instances
    std::memcpy(buffer.data() + header.m_FirstInstanceOffset, instances.data(),
                (sizeof(AdfInstance) * instances.size()));

    // write the types
    uint32_t cur_type_offset = 0;
    for (const auto type : types) {
        const uint32_t size = type->DataSize();
        std::memcpy(buffer.data() + header.m_FirstTypeOffset + cur_type_offset, (char *)type, size);
        cur_type_offset += size;
    }

    // write the string hashes
    uint32_t cur_string_hash_offset = 0;
    for (auto it = StringHashes.begin(); it != StringHashes.end(); ++it) {
        const auto  hash   = (*it).first;
        const auto &string = (*it).second;

        // write the string
        std::memcpy(buffer.data() + header.m_FirstStringHashOffset + cur_string_hash_offset, string.data(),
                    string.size());
        cur_string_hash_offset += (string.length() + 1);

        // write the string hash
        // NOTE: format uses uint64_t NOT uint32_t
        std::memcpy(buffer.data() + header.m_FirstStringHashOffset + cur_string_hash_offset, &hash, sizeof(hash));
        cur_string_hash_offset += sizeof(uint64_t);
    }

    // write the strings
    uint32_t total_lengths     = (sizeof(uint8_t) * total_strings);
    uint32_t cur_string_offset = 0;
    for (uint32_t i = 0; i < Strings.size(); ++i) {
        const auto &string = Strings[i];
        uint8_t     length = string.length();

        // write the string length
        std::memcpy(buffer.data() + header.m_FirstStringDataOffset + (sizeof(uint8_t) * i), &length, sizeof(length));

        // write the string
        std::memcpy(buffer.data() + header.m_FirstStringDataOffset + total_lengths + cur_string_offset, string.data(),
                    length + 1);

        cur_string_offset += length + 1;
    }

    // clear temp storage
    Strings.clear();
    StringHashes.clear();
    StringRefs.clear();
    RefsToFix.clear();

    auto new_filename = filename.parent_path() / adf_filename;
    return callback(true, new_filename, buffer);
}

static void WriteInstance(tinyxml2::XMLPrinter &printer, ava::AvalancheDataFormat::ADF *adf,
                          const ava::AvalancheDataFormat::AdfHeader *header,
                          const ava::AvalancheDataFormat::AdfType *type, const char *data, uint32_t offset = 0)
{
    using namespace ava::AvalancheDataFormat;

    // @NOTE - tinyxml2 uses .8g precision, avalanche use .9g.
    static auto PushHigherPrecisionFloat = [](tinyxml2::XMLPrinter &printer, float value) {
        static auto TIXML_SNPRINTF = [](char *buffer, size_t size, const char *format, ...) {
            va_list va;
            va_start(va, format);
            vsnprintf_s(buffer, size, _TRUNCATE, format, va);
            va_end(va);
        };

        char buf[200];
        TIXML_SNPRINTF(buf, 200, "%.9g", value);
        printer.PushText(buf, false);
    };

    switch (type->m_Type) {
        case ADF_TYPE_SCALAR: {
            // clang-format off
            switch (type->m_ScalarType) {
                case ADF_SCALARTYPE_SIGNED:
                    switch (type->m_Size) {
                        case sizeof(int8_t):   printer.PushText(*(int8_t*)&data[offset]); break;
                        case sizeof(int16_t):  printer.PushText(*(int16_t*)&data[offset]); break;
                        case sizeof(int32_t):  printer.PushText(*(int32_t*)&data[offset]); break;
                        case sizeof(int64_t):  printer.PushText(*(int64_t*)&data[offset]); break;
                    }
                    break;
                case ADF_SCALARTYPE_UNSIGNED:
                    switch (type->m_Size) {
                        case sizeof(uint8_t):  printer.PushText(*(uint8_t*)&data[offset]); break;
                        case sizeof(uint16_t): printer.PushText(*(uint16_t*)&data[offset]); break;
                        case sizeof(uint32_t): printer.PushText(*(uint32_t*)&data[offset]); break;
                        case sizeof(uint64_t): printer.PushText(*(uint64_t*)&data[offset]); break;
                    }
                    break;
                case ADF_SCALARTYPE_FLOAT:
                    switch (type->m_Size) {
                        case sizeof(float):    PushHigherPrecisionFloat(printer, *(float*)&data[offset]); break;
                        case sizeof(double):   printer.PushText(*(double*)&data[offset]); break;
                    }
                    break;
            }
            // clang-format on

            break;
        }

        case ADF_TYPE_STRUCT: {
            printer.OpenElement("struct");
            printer.PushAttribute("type", adf->GetString(type->m_Name).c_str());

            // write struct members
            for (uint32_t i = 0; i < type->m_MemberCount; ++i) {
                const AdfMember &member      = type->m_Members[i];
                const AdfType *  member_type = adf->FindType(member.m_TypeHash);

                if (!member_type) {
                    SPDLOG_ERROR("Unknown member type in struct! (Member={}, TypeHash={:x})",
                                 adf->GetString(member.m_Name), member.m_TypeHash);
                    continue;
                }

                printer.OpenElement("member");
                printer.PushAttribute("name", adf->GetString(member.m_Name).c_str());

                if (member_type->m_Type == ADF_TYPE_BITFIELD) {
                    const uint32_t count    = member_type->m_BitCount;
                    const uint64_t bit_data = *(uint64_t *)&data[(offset + member.m_Offset)];

                    const int64_t v26 = (1 << count);
                    int64_t       v27 = (v26 - 1) & (bit_data >> member.m_BitOffset);

                    if (member_type->m_ScalarType == ADF_SCALARTYPE_SIGNED
                        && _bittest64((long long *)&v27, (count - 1))) {
                        v27 -= v26;
                    }

                    // @TODO: signed values seem wrong? showing -4 instead of 4
                    printer.PushText(v27);
                } else {
                    PushPrimitiveTypeAttribute(printer, member_type->m_TypeHash);
                    WriteInstance(printer, adf, header, member_type, data, (offset + member.m_Offset));
                }

                printer.CloseElement();
            }

            printer.CloseElement();
            break;
        }

        case ADF_TYPE_POINTER:
        case ADF_TYPE_DEFERRED: {
            const uint32_t rel_offset = *(uint32_t *)&data[offset];
            uint32_t       type_hash  = 0xDEFE88ED;

            if (type->m_Type == ADF_TYPE_POINTER) {
                type_hash = type->m_SubTypeHash;
            } else if (rel_offset) {
                type_hash = *(uint32_t *)&data[offset + 8];
            }

            const AdfType *deferred_type = adf->FindType(type_hash);
            if (deferred_type) {
                printer.OpenElement("pointer");
                printer.PushAttribute("type", type_hash);

                if (rel_offset) {
                    WriteInstance(printer, adf, header, deferred_type, data, rel_offset);
                }

                printer.CloseElement();
            }

            break;
        }

        case ADF_TYPE_ARRAY: {
            const uint32_t rel_offset = *(uint32_t *)&data[offset];
            const uint32_t count      = *(uint32_t *)&data[offset + 8];

            const AdfType *sub_type = adf->FindType(type->m_SubTypeHash);
            if (!sub_type) {
                SPDLOG_ERROR("Unknown SubType in array! (Type={}, SubTypeHash={:x})", adf->GetString(type->m_Name),
                             type->m_SubTypeHash);
                break;
            }

            printer.OpenElement("array");
            PushPrimitiveTypeAttribute(printer, sub_type->m_TypeHash);

            // @TODO: don't write the count!
            printer.PushAttribute("count", count);

            for (uint32_t i = 0; i < count; ++i) {
                const uint32_t el_offset = (rel_offset + (sub_type->m_Size * i));
                WriteInstance(printer, adf, header, sub_type, data, el_offset);

                // add a space if it's not the last element
                if (sub_type->m_Type == ADF_TYPE_SCALAR && (i != (count - 1))) {
                    printer.PushText(" ");
                }
            }

            printer.CloseElement();
            break;
        }

        case ADF_TYPE_INLINE_ARRAY: {
            const AdfType *sub_type = adf->FindType(type->m_SubTypeHash);
            if (!sub_type) {
                SPDLOG_ERROR("Unknown SubType in inline_array! (Type={}, SubTypeHash={:x})",
                             adf->GetString(type->m_Name), type->m_SubTypeHash);
                break;
            }

            uint32_t size = sub_type->m_Size;
            if (sub_type->m_Type == ADF_TYPE_STRING || sub_type->m_Type == ADF_TYPE_POINTER) {
                size = 8;
            }

            printer.OpenElement("inline_array");
            PushPrimitiveTypeAttribute(printer, sub_type->m_TypeHash);

            for (uint32_t i = 0; i < type->m_ArraySize; ++i) {
                WriteInstance(printer, adf, header, sub_type, data, (offset + (size * i)));

                // add a space if it's not the last element
                if (sub_type->m_Type == ADF_TYPE_SCALAR && (i != (type->m_ArraySize - 1))) {
                    printer.PushText(" ");
                }
            }

            printer.CloseElement();
            break;
        }

        case ADF_TYPE_STRING: {
            const uint32_t rel_offset = *(uint32_t *)&data[offset];
            const char *   value      = &data[rel_offset];
            printer.PushText(value);
            break;
        }

        case ADF_TYPE_ENUM: {
            // @TODO: because enums are also used for flags, the "index" is actually the value, and multiple members
            // can be OR'd together meaning the index might sometimes be greater than the member count

            // idealy, we would want visually show this instead of writing the enum value directly
            // e.g.: FLAG1 | FLAG6 | FLAG8 etc..

            const uint32_t value = *(uint32_t *)&data[offset];
            printer.PushText(value);
            break;
        }

        case ADF_TYPE_STRING_HASH: {
            printer.OpenElement("stringhash");

            const uint32_t hash = *(uint32_t *)&data[offset];
            printer.PushText(adf->HashLookup(hash));

            printer.CloseElement();
            break;
        }
    }
}

#define XmlPushAttribute(k, v)                                                                                         \
    if (v) {                                                                                                           \
        printer.PushAttribute(k, v);                                                                                   \
    }

static void AddTypeLibraries(ava::AvalancheDataFormat::ADF *adf, const std::filesystem::path &extension)
{
    static auto ReadTypeLibrary = [](ava::AvalancheDataFormat::ADF *adf, const std::string &filename) {
        try {
            AvalancheArchive *   arc = FileLoader::Get()->GetAdfTypeLibraries();
            std::vector<uint8_t> out_buffer;
            ava::StreamArchive::ReadEntry(arc->GetBuffer(), arc->GetEntries(), filename, &out_buffer);
            adf->AddTypes(out_buffer);
        } catch (const std::exception &e) {
            SPDLOG_ERROR(e.what());
#ifdef _DEBUG
            __debugbreak();
#endif
        }
    };

    if (extension == ".blo_adf" || extension == ".flo_adf" || extension == ".epe_adf") {
        ReadTypeLibrary(adf, "AttachedEffects.adf");
        ReadTypeLibrary(adf, "Damageable.adf");
        ReadTypeLibrary(adf, "DamageController.adf");
        ReadTypeLibrary(adf, "LocationGameObjectAdf.adf");
        ReadTypeLibrary(adf, "PhysicsGameObject.adf");
        ReadTypeLibrary(adf, "RigidObject.adf");
        ReadTypeLibrary(adf, "TargetSystem.adf");
    }
}

static void DoExport(tinyxml2::XMLPrinter &printer, const std::filesystem::path &filename,
                     const std::vector<uint8_t> &buffer)
{
    using namespace ava::AvalancheDataFormat;

    // parse header to get file description
    AdfHeader   header{};
    const char *description = "";
    ParseHeader(buffer, &header, &description);

    // init adf & add type libraries
    ADF adf{buffer};
    AddTypeLibraries(&adf, filename.extension());

    // write adf element
    printer.PushHeader(false, true);
    printer.PushComment(" File generated by " VER_PRODUCTNAME_STR " v" VER_PRODUCT_VERSION_STR " ");
    printer.PushComment(" https://github.com/aaronkirkham/jc-model-renderer ");
    printer.OpenElement("adf");
    XmlPushAttribute("extension", filename.extension().string().c_str());
    XmlPushAttribute("version", header.m_Version);
    XmlPushAttribute("flags", header.m_Flags);

    // write description (needed if we reimport later)
    if (strlen(description) > 0) {
        std::string desc(description);
        util::replace(desc, "\n", ", ");
        XmlPushAttribute("library", desc.c_str());
    }

    // write instances
    for (decltype(header.m_InstanceCount) i = 0; i < header.m_InstanceCount; ++i) {
        SInstanceInfo instance_info{};
        adf.GetInstance(i, &instance_info);

        printer.OpenElement("instance");
        XmlPushAttribute("name", instance_info.m_Name);
        XmlPushAttribute("typehash", instance_info.m_TypeHash);

        // type is missing, don't write instance
        if (instance_info.m_InstanceSize > 0) {
            // @TEMP: until we generate the offset/size automagically
            {
                auto payload_offset = (uint32_t)((uint8_t *)instance_info.m_Instance - adf.GetBuffer()->data());
                XmlPushAttribute("offset", payload_offset);
                XmlPushAttribute("size", instance_info.m_InstanceSize);
            }

            // @TEMP: unknown value after each payload, figure it out!
            if (header.m_Flags & E_ADF_HEADER_FLAG_RELATIVE_OFFSETS_EXISTS) {
                const uint32_t unknown = *(uint32_t *)((char *)instance_info.m_Instance + instance_info.m_InstanceSize);
                XmlPushAttribute("unknown", unknown);
            }

            // write instance type members
            // @NOTE: type is guaranteed to be valid if instancesize > 0.
            const AdfType *type = adf.FindType(instance_info.m_TypeHash);
            WriteInstance(printer, &adf, &header, type, (const char *)instance_info.m_Instance);
        } else {
            SPDLOG_WARN("The instance \"{}\" couldn't be written because the type {0:x} is missing.",
                        instance_info.m_Name, instance_info.m_TypeHash);
            printer.PushComment(" This instance couldn't be written because the type is missing. ");
        }

        printer.CloseElement();
    }

    // write types
    if (header.m_TypeCount > 0) {
        printer.OpenElement("types");
        printer.PushComment(" !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! ");
        printer.PushComment(" !!! Internal ADF types. Do not change anything in here !!! ");
        printer.PushComment(" !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! ");

        uint32_t types_accumulated_size = 0;
        for (uint32_t i = 0; i < header.m_TypeCount; ++i) {
            const uint32_t type_hash = *(uint32_t *)((char *)&buffer[header.m_FirstTypeOffset] + types_accumulated_size
                                                     + offsetof(AdfType, m_TypeHash));
            const AdfType *type      = adf.FindType(type_hash);
            if (!type) {
                // @TODO: throw or push comment in output XML?
                //        this is non-fatal so continuing here is possible.
                throw std::runtime_error("ADF type definition is not availabe!");
            }

            printer.OpenElement("type");
            XmlPushAttribute("name", adf.GetString(type->m_Name).c_str());
            XmlPushAttribute("type", type->m_Type);
            XmlPushAttribute("size", type->m_Size);
            XmlPushAttribute("align", type->m_Align);
            XmlPushAttribute("typehash", type->m_TypeHash);
            XmlPushAttribute("flags", type->m_Flags);
            XmlPushAttribute("scalartype", type->m_ScalarType);
            XmlPushAttribute("subtypehash", type->m_SubTypeHash);
            XmlPushAttribute("count", type->m_ArraySize);

            // write all type members
            if (type->m_Type == EAdfType::ADF_TYPE_STRUCT || type->m_Type == EAdfType::ADF_TYPE_ENUM) {
                const bool is_enum = (type->m_Type == EAdfType::ADF_TYPE_ENUM);
                for (decltype(type->m_MemberCount) y = 0; y < type->m_MemberCount; ++y) {
                    printer.OpenElement("member");

                    if (is_enum) {
                        const AdfEnum &enum_member = type->Enum(y);
                        XmlPushAttribute("name", adf.GetString(enum_member.m_Name).c_str());
                        XmlPushAttribute("value", enum_member.m_Value);
                    } else {
                        const AdfMember &member = type->m_Members[y];
                        XmlPushAttribute("name", adf.GetString(member.m_Name).c_str());
                        XmlPushAttribute("typehash", member.m_TypeHash);
                        XmlPushAttribute("align", member.m_Align);
                        XmlPushAttribute("offset", member.m_Offset);
                        XmlPushAttribute("bitoffset", member.m_BitOffset);
                        XmlPushAttribute("flags", member.m_Flags);
                        XmlPushAttribute("default", member.m_DefaultValue);
                    }

                    printer.CloseElement();
                }
            }

            printer.CloseElement();

            types_accumulated_size += (uint32_t)type->DataSize();
        }

        printer.CloseElement();
    }

    printer.CloseElement();
}

void ADF2XML::Export(const std::filesystem::path &filename, const std::filesystem::path &to,
                     ExportFinishedCallback callback)
{

    auto &path = to / filename.filename();
    path += GetExportExtension();

    FileLoader::Get()->ReadFile(filename, [&, filename, path, callback](bool success, std::vector<uint8_t> data) {
        if (success) {
            try {
                tinyxml2::XMLPrinter printer;
                DoExport(printer, filename, data);

                // write outfile file
                std::ofstream out_stream(path);
                out_stream << printer.CStr();
                out_stream.close();
                return callback(true);
            } catch (const std::exception &e) {
                SPDLOG_ERROR(e.what());
            }

            return callback(false);
        } else {
            return callback(false);
        }
    });
}
}; // namespace ImportExport

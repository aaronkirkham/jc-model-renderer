#include <jc3/formats/AvalancheArchive.h>
#include <jc3/FileLoader.h>
#include <jc3/formats/RenderBlockModel.h>
#include <Window.h>
#include <graphics/UI.h>

std::recursive_mutex Factory<AvalancheArchive>::InstancesMutex;
std::map<uint32_t, std::shared_ptr<AvalancheArchive>> Factory<AvalancheArchive>::Instances;

AvalancheArchive::AvalancheArchive(const fs::path& file)
    : m_File(file)
{
    // read the stream archive
    FileLoader::Get()->ReadStreamArchive(file, [&](std::unique_ptr<StreamArchive_t> archive) {
        assert(archive);
        m_StreamArchive = std::move(archive);

        // parse the file list
        m_FileList = std::make_unique<DirectoryList>();
        m_FileList->Parse(m_StreamArchive.get());
    });
}

AvalancheArchive::AvalancheArchive(const fs::path& filename, const FileBuffer& buffer)
    : m_File(filename)
{
    // read the stream archive
    FileLoader::Get()->ReadStreamArchive(filename, std::move(buffer), [&](std::unique_ptr<StreamArchive_t> archive) {
        assert(archive);
        m_StreamArchive = std::move(archive);

        // parse the file list
        m_FileList = std::make_unique<DirectoryList>();
        m_FileList->Parse(m_StreamArchive.get());
    });
}

void AvalancheArchive::AddFile(const fs::path& filename, const FileBuffer& data)
{
    assert(m_StreamArchive);
    assert(m_FileList);

    // replace backslashes
    auto name = filename.string();
    std::replace(name.begin(), name.end(), '\\', '/');

    if (!m_StreamArchive->HasFile(name)) {
        m_FileList->Add(name);
    }

    m_StreamArchive->AddFile(name, data);
    m_HasUnsavedChanged = true;
}

void AvalancheArchive::AddDirectory(const fs::path& filename, const fs::path& root)
{
    // TODO: only add files which are a supported format
    // generic textures, models, etc

    if (fs::is_directory(filename)) {
        for (const auto& f : fs::directory_iterator(filename)) {
            AddDirectory(f.path(), root);
        }
    }
    else {
        // TEMP (no relative/lexically_relative in experimental::fs, but moving to ::filesystem seems to break things)
        std::filesystem::path fn(filename);
        std::filesystem::path r(root);

        // strip the root directory from the path
        auto name = fn.lexically_relative(r).string();
        const auto size = fs::file_size(filename);

        FileBuffer buffer;
        buffer.resize(size);
        std::ifstream stream(filename, std::ios::binary);
        assert(!stream.fail());
        stream.read((char *)buffer.data(), size);
        stream.close();

        AddFile(name, buffer);
    }
}

bool AvalancheArchive::HasFile(const fs::path& filename)
{
    auto find_it = std::find_if(m_StreamArchive->m_Files.begin(), m_StreamArchive->m_Files.end(), [&](const StreamArchiveEntry_t& entry) {
        return entry.m_Filename == filename || entry.m_Filename.find(filename.string()) != std::string::npos;
    });

    return find_it != m_StreamArchive->m_Files.end();
}

void AvalancheArchive::ReadFileCallback(const fs::path& filename, const FileBuffer& data)
{
    AvalancheArchive::make(filename, data);
}

bool AvalancheArchive::SaveFileCallback(const fs::path& filename, const fs::path& directory)
{
    DEBUG_LOG("AvalancheArchive::SaveFileCallback");

    auto archive = AvalancheArchive::get(filename.string());
    if (archive) {
        assert(archive->m_StreamArchive);

        std::stringstream status_text;
        status_text << "Repacking \"" << filename.filename() << "\"...";
        const auto status_text_id = UI::Get()->PushStatusText(status_text.str());

        std::thread([&, archive, filename, directory, status_text_id] {
            // generate the .ee.toc
            auto toc = filename; toc.replace_extension(".ee.toc");
            const auto& toc_path = directory / toc.filename();
            FileLoader::Get()->WriteTOC(toc_path, archive->m_StreamArchive.get());

            // generate the .ee
            const auto& ee_path = directory / filename.filename();
            std::ofstream stream(ee_path, std::ios::binary);
            FileLoader::Get()->CompressArchive(stream, archive->m_StreamArchive.get());
            stream.close();

            UI::Get()->PopStatusText(status_text_id);
        }).detach();

        return true;
    }

    return false;
}
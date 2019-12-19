#include <AvaFormatLib.h>
#include <sstream>

#include <rapidjson/document.h>

#include "../vendor/ava-format-lib/include/archives/legacy/archive_table.h"
#include "../vendor/ava-format-lib/include/util/byte_array_buffer.h"
#include "../vendor/ava-format-lib/include/util/byte_vector_writer.h"

#include "directory_list.h"
#include "file_loader.h"
#include "name_hash_lookup.h"
#include "settings.h"
#include "window.h"

#include "graphics/dds_texture_loader.h"
#include "graphics/texture.h"
#include "graphics/texture_manager.h"
#include "graphics/ui.h"

#include "game/formats/avalanche_archive.h"
#include "game/formats/render_block_model.h"
#include "game/formats/runtime_container.h"
#include "game/render_block_factory.h"
#include "game/types.h"

#include "import_export/import_export_manager.h"

static uint64_t g_ArchiveLoadCount = 0;
extern bool     g_IsJC4Mode;

std::unordered_map<uint32_t, std::string> NameHashLookup::LookupTable;

FileLoader::FileLoader()
{
    // init the namehash lookup table
    NameHashLookup::Init();

    // load ADF type libraries
    std::vector<uint8_t> buffer;
    if (Window::Get()->LoadInternalResource(103, &buffer)) {
        m_AdfTypeLibraries = std::make_unique<AvalancheArchive>("adf-type-libraries.ee", buffer, true);
    }

    // TODO: move the events to a better location?
    // trigger the file type callbacks
    UI::Get()->Events().FileTreeItemSelected.connect([&](const std::filesystem::path& file, AvalancheArchive* archive) {
        // do we have a registered callback for this file type?
        if (m_FileReadHandlers.find(file.extension().string()) != m_FileReadHandlers.end()) {
            ReadFile(file, [&, file](bool success, std::vector<uint8_t> data) {
                if (success) {
                    for (const auto& fn : m_FileReadHandlers[file.extension().string()]) {
                        fn(file, std::move(data), false);
                    }
                } else {
                    SPDLOG_ERROR("(FileTreeItemSelected) Failed to load \"{}\"", file.string());

                    std::string error = "Failed to load \"" + file.filename().string() + "\".\n\n";
                    error += "Make sure you have selected the correct Just Cause directory.";
                    Window::Get()->ShowMessageBox(error);
                }
            });
        } else {
            SPDLOG_ERROR("(FileTreeItemSelected) Unknown file type extension \"{}\" (\"{}\")",
                         file.extension().string(), file.string());

            std::string error = "Unable to read the \"" + file.extension().string() + "\" extension.\n\n";
            error += "Want to help? Check out our GitHub page for information on how to contribute.";
            Window::Get()->ShowMessageBox(error);
        }
    });

    // save file
    UI::Get()->Events().SaveFileRequest.connect(
        [&](const std::filesystem::path& file, const std::filesystem::path& directory, bool is_dropzone) {
            bool was_handled = false;

            // keep the full path if we are saving to the dropzone
            std::filesystem::path path = directory;
            path /= (is_dropzone ? file : file.filename());

            std::filesystem::create_directories(path.parent_path());

            // try use a handler
            if (m_FileSaveHandlers.find(file.extension().string()) != m_FileSaveHandlers.end()) {
                for (const auto& fn : m_FileSaveHandlers[file.extension().string()]) {
                    was_handled = fn(file, path);
                }
            }

            // no handlers, fallback to just reading the raw data
            if (!was_handled) {
                FileLoader::Get()->ReadFile(
                    file,
                    [&, file, path](bool success, std::vector<uint8_t> data) {
                        if (success) {
                            // write the file data
                            std::ofstream stream(path, std::ios::binary);
                            if (!stream.fail()) {
                                stream.write((char*)data.data(), data.size());
                                stream.close();
                                return;
                            }
                        }

                        SPDLOG_ERROR("(SaveFileRequest) Failed to save \"{}\"", file.filename().string());
                        Window::Get()->ShowMessageBox("Failed to save \"" + file.filename().string() + "\".");
                    },
                    ReadFileFlags_SkipTextureLoader);
            }
        });

    // import file
    UI::Get()->Events().ImportFileRequest.connect([&](IImportExporter* importer, ImportFinishedCallback callback) {
        std::string filter_extension = "*";
        filter_extension.append(importer->GetExportExtension());

        std::string filter_name = importer->GetExportName();
        filter_name.append(" (");
        filter_name.append(filter_extension);
        filter_name.append(")");

        FileSelectionParams params{};
        params.Extension = importer->GetExportExtension();
        params.Filters.emplace_back(FileSelectionFilter{filter_name, filter_extension});

        const auto& selected = Window::Get()->ShowFileSelecton("Select a file to import", params);
        if (!selected.empty()) {
            SPDLOG_INFO("(ImportFileRequest) Want to import file \"{}\"", selected.string());
            std::thread([&, importer, selected, callback] { importer->Import(selected, callback); }).detach();
        }
    });

    // export file
    UI::Get()->Events().ExportFileRequest.connect([&](const std::filesystem::path& file, IImportExporter* exporter) {
        const auto& selected = Window::Get()->ShowFolderSelection("Select a folder to export the file to");
        if (!selected.empty()) {
            auto _exporter = exporter;
            if (!exporter) {
                const auto& exporters = ImportExportManager::Get()->GetExportersFor(file.extension().string());
                if (exporters.size() > 0) {
                    _exporter = exporters.at(0);
                }
            }

            // if we have a valid exporter, read the file and export it
            if (_exporter) {
                std::string status_text    = "Exporting \"" + file.generic_string() + "\"...";
                const auto  status_text_id = UI::Get()->PushStatusText(status_text);

                std::thread([&, file, selected, status_text_id] {
                    _exporter->Export(file, selected, [&, file, status_text_id](bool success) {
                        UI::Get()->PopStatusText(status_text_id);

                        if (!success) {
                            SPDLOG_ERROR("(ExportFileRequest) Failed to export \"{}\"", file.filename().string());
                            Window::Get()->ShowMessageBox("Failed to export \"" + file.filename().string() + "\".");
                        }
                    });
                }).detach();
            }
        }
    });
}

FileLoader::~FileLoader()
{
    Shutdown();
}

void FileLoader::Init()
{
    Shutdown();

    // load oo2core_7_win64.dll
    if (g_IsJC4Mode) {
        try {
            std::filesystem::path filename = Window::Get()->GetJustCauseDirectory();
            filename /= "oo2core_7_win64.dll";

            ava::Oodle::LoadLib(filename.string().c_str());
        } catch (const std::exception& e) {
            SPDLOG_ERROR("Failed to load oo2core_7_win64.dll. ({})", e.what());
            Window::Get()->ShowMessageBox(
                "Failed to load oo2core_7_win64.dll.\n\nPlease make sure your Just Cause 4 directory is valid.",
                MB_ICONERROR | MB_OK);
        }
    }

    // init the filelist
    m_FileList          = std::make_unique<DirectoryList>();
    m_DictionaryLoading = true;

    const auto status_text_id = UI::Get()->PushStatusText("Loading dictionary...");
    std::thread([this, status_text_id] {
        m_Dictionary.clear();

        try {
            std::vector<uint8_t> buffer;
            if (!Window::Get()->LoadInternalResource(g_IsJC4Mode ? 256 : 128, &buffer)) {
                throw std::runtime_error("LoadInternalResource failed.");
            }

            byte_array_buffer         buf(buffer);
            std::istream              stream(&buf);
            rapidjson::IStreamWrapper stream_wrapper(stream);

            rapidjson::Document doc;
            doc.ParseStream(stream_wrapper);

            assert(doc.IsObject());
            m_Dictionary.reserve(doc.GetObjectA().MemberCount());

            for (auto itr = doc.MemberBegin(); itr != doc.MemberEnd(); ++itr) {
                const uint32_t name_hash = ava::hashlittle(itr->name.GetString());

                // add to file list
                m_FileList->Add(itr->name.GetString());

                std::vector<std::string> _paths;
                for (auto paths_itr = itr->value.Begin(); paths_itr != itr->value.End(); ++paths_itr) {
                    _paths.push_back(paths_itr->GetString());
                }

                m_Dictionary[name_hash] = std::pair{itr->name.GetString(), std::move(_paths)};
            }
        } catch (const std::exception& e) {
            SPDLOG_ERROR("Failed to load file list dictionary. ({})", e.what());

            std::string error = "Failed to load file list dictionary. (";
            error += e.what();
            error += ")\n\n";
            error += "Some features will be disabled.";
            Window::Get()->ShowMessageBox(error);
        }

        UI::Get()->PopStatusText(status_text_id);
        m_DictionaryLoading = false;
    }).detach();
}

void FileLoader::Shutdown()
{
    ava::Oodle::UnloadLib();
    m_FileList = nullptr;
}

void FileLoader::ReadFile(const std::filesystem::path& filename, ReadFileResultCallback callback, uint8_t flags)
{
    uint64_t status_text_id = 0;

    // if the path is absolute, read straight from disk
    if (filename.is_absolute()) {
        return ReadFileFromDisk(filename, callback);
    }

    if (!UseBatches) {
        std::string status_text = "Reading \"" + filename.generic_string() + "\"...";
        status_text_id          = UI::Get()->PushStatusText(status_text);
    }

#if 0
    // are we trying to load textures?
    if (filename.extension() == ".dds" || filename.extension() == ".ddsc" || filename.extension() == ".hmddsc"
        || filename.extension() == ".atx1" || filename.extension() == ".atx2") {
        const auto& texture = TextureManager::Get()->GetTexture(filename, TextureManager::TextureCreateFlags_NoCreate);
        if (texture && texture->IsLoaded()) {
            UI::Get()->PopStatusText(status_text_id);
            return callback(true, texture->GetBuffer());
        }

        // try load the texture
        if (!(flags & ReadFileFlags_SkipTextureLoader)) {
            UI::Get()->PopStatusText(status_text_id);
            return ReadTexture(filename, callback);
        }
    }
#endif

    // check any loaded archives for the file
    const auto& [archive, entry] = GetStreamArchiveFromFile(filename);
    if (archive && (entry.m_Offset != 0 && entry.m_Offset != -1)) {
        std::vector<uint8_t> buffer;
        ava::StreamArchive::ReadEntry(archive->GetBuffer(), entry, &buffer);

        UI::Get()->PopStatusText(status_text_id);
        return callback(true, std::move(buffer));
    }
#ifdef DEBUG
    else if (archive && (entry.m_Offset == 0 || entry.m_Offset == -1)) {
        SPDLOG_INFO("\"{}\" exists in archive but has been patched. Will read the patch version instead.",
                    filename.filename().string());
    }
#endif

    // should we use file batching?
    if (UseBatches) {
        SPDLOG_TRACE("Using batches for \"{}\"", filename.string());
        return ReadFileBatched(filename, callback);
    }

    // finally, lets read it directory from the arc file
    std::thread([this, filename, callback, status_text_id] {
        while (m_DictionaryLoading) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }

        const auto& [directory_name, archive_name, namehash] = LocateFileInDictionary(filename);
        if (!directory_name.empty()) {
            std::vector<uint8_t> buffer;
            if (ReadFileFromArchive(directory_name, archive_name, namehash, &buffer)) {
                UI::Get()->PopStatusText(status_text_id);
                return callback(true, std::move(buffer));
            }
        }

        UI::Get()->PopStatusText(status_text_id);
        return callback(false, {});
    }).detach();
}

void FileLoader::ReadFileBatched(const std::filesystem::path& filename, ReadFileResultCallback callback)
{
    std::lock_guard<std::recursive_mutex> _lk{m_BatchesMutex};
    m_PathBatches[filename.string()].emplace_back(callback);
}

void FileLoader::RunFileBatches()
{
    using namespace ava;

    std::thread([&] {
        std::lock_guard<std::recursive_mutex> _lk{m_BatchesMutex};

        for (const auto& path : m_PathBatches) {
            const auto& [directory_name, archive_name, namehash] = LocateFileInDictionary(path.first);
            if (!directory_name.empty()) {
                const auto& archive_path = (directory_name + "/" + archive_name);

                for (const auto& callback : path.second) {
                    m_Batches[archive_path][namehash].emplace_back(callback);
                }
            }
            // nothing found, callback
            else {
                SPDLOG_ERROR("Didn't find {}", path.first);
                for (const auto& callback : path.second) {
                    callback(false, {});
                }
            }
        }

#ifdef DEBUG
        for (const auto& batch : m_Batches) {
            int c = 0;
            for (const auto& file : batch.second) {
                c += file.second.size();
            }

            SPDLOG_INFO("Will read {} files from \"{}\"", c, batch.first);
        }
#endif

        // iterate over all the archives
        for (const auto& batch : m_Batches) {
            const auto& archive_path = batch.first;
            const auto  sep          = archive_path.find_last_of("/");

            // split the directory and archive
            const auto& directory = archive_path.substr(0, sep);
            const auto& archive   = archive_path.substr(sep + 1, archive_path.length());

            // get the path names
            auto&      jc_directory = Window::Get()->GetJustCauseDirectory();
            const auto tab_file     = jc_directory / directory / (archive + ".tab");
            const auto arc_file     = jc_directory / directory / (archive + ".arc");

            // ensure the tab file exists
            if (!std::filesystem::exists(tab_file)) {
                SPDLOG_ERROR("Can't find TAB file (\"{}\")", tab_file.string());
                continue;
            }

            // ensure the arc file exists
            if (!std::filesystem::exists(arc_file)) {
                SPDLOG_ERROR("Can't find ARC file (\"{}\")", arc_file.string());
                continue;
            }

            std::vector<ArchiveTable::TabEntry>           entries;
            std::vector<ArchiveTable::TabCompressedBlock> blocks;
            if (ReadArchiveTable(tab_file, &entries, &blocks)) {
                // open the arc file
                std::ifstream stream(arc_file, std::ios::binary);
                if (stream.fail()) {
                    SPDLOG_ERROR("Invalid ARC file stream!");
                    continue;
                }

#ifdef DEBUG
                g_ArchiveLoadCount++;
                SPDLOG_INFO("Total archive load count: {}", g_ArchiveLoadCount);
#endif

                // iterate over all the files
                for (const auto& file : batch.second) {
                    const auto it =
                        std::find_if(entries.begin(), entries.end(), [&](const ArchiveTable::TabEntry& entry) {
                            return entry.m_NameHash == file.first;
                        });

                    if (it != entries.end()) {
                        const auto& entry = (*it);

                        //
                        stream.seekg(entry.m_Offset);

                        // read the file buffer
                        std::vector<uint8_t> buffer(entry.m_Size);
                        stream.read((char*)buffer.data(), entry.m_Size);

                        // trigger the callbacks
                        for (const auto& callback : file.second) {
                            callback(true, std::move(buffer));
                        }
                    } else {
                        SPDLOG_ERROR("Didn't find {}", file.first);

                        for (const auto& callback : file.second) {
                            callback(false, {});
                        }
                    }
                }

                stream.close();
            }
        }

        // clear the batches
        m_PathBatches.clear();
        m_Batches.clear();
    }).detach();
}

void FileLoader::ReadFileFromDisk(const std::filesystem::path& filename, ReadFileResultCallback callback)
{
    std::thread([&, filename, callback] {
        if (!std::filesystem::exists(filename)) {
            SPDLOG_ERROR("Input file doesn't exists. \"{}\"", filename.string());
            return callback(false, {});
        }

        std::ifstream stream(filename, std::ios::binary);
        if (stream.fail()) {
            SPDLOG_ERROR("Failed to create file stream!");
            return callback(false, {});
        }

        // read the file
        const auto           size = std::filesystem::file_size(filename);
        std::vector<uint8_t> buffer(size);
        stream.read((char*)buffer.data(), size);
        stream.close();

        return callback(true, std::move(buffer));
    }).detach();
}

void FileLoader::ReadFileFromDiskAndRunHandlers(const std::filesystem::path& filename)
{
    bool        is_unhandled = false;
    const auto& extension    = filename.extension().string();
    auto&       importers    = ImportExportManager::Get()->GetImportersFrom(extension, true);
    const auto& exporters    = ImportExportManager::Get()->GetExportersFor(extension);

    // do we have a registered callback for this file type?
    if (m_FileReadHandlers.find(extension) != m_FileReadHandlers.end()) {
        ReadFileFromDisk(filename, [&, filename, extension](bool success, std::vector<uint8_t> data) {
            if (success) {
                for (const auto& fn : m_FileReadHandlers[extension]) {
                    fn(filename.filename(), std::move(data), true);
                }
            }
        });
    } else if (importers.size() > 0) {
        static const auto finished_callback = [](bool success, std::filesystem::path filename, std::any data) {
            if (success) {
                // NOTE: this kind of assumes the data will always be binary data
                // because the importer will always have drag-and-drop support (see
                // ReadFileFromDiskAndRunHandlers)

                // TODO: this can be improved.

                auto buffer = std::any_cast<std::vector<uint8_t>>(data);

                std::ofstream stream(filename, std::ios::binary);
                assert(!stream.fail());
                stream.write((char*)buffer.data(), buffer.size());
                stream.close();
            }
        };

        // if we have more than 1 importer, try find one for the real extension
        // NOTE: files are exported with <name>.<real extension>.<export extension>, we're trying to find importers
        // which deal with the real extension
        if (importers.size() > 1) {
            // filter importers based on filename extensions
            std::vector<IImportExporter*> final_importers;
            for (const auto importer : importers) {
                for (const auto& extension : importer->GetImportExtension()) {
                    if (filename.filename().string().find(extension) != std::string::npos) {
                        final_importers.push_back(importer);
                    }
                }
            }

            if (final_importers.size() != 0) {
                importers = std::move(final_importers);
            }
        }

        const size_t total = importers.size();
        if (total == 0) {
            is_unhandled = true;
        } else if (total == 1) {
            importers[0]->Import(filename, finished_callback);
        } else {
            UI::Get()->ShowSelectImporter(filename, importers, finished_callback);
        }
    } else if (exporters.size() > 0) {
        auto to = filename.parent_path();
        exporters[0]->Export(filename, to, [&](bool success) {});
    } else {
        is_unhandled = true;
    }

    if (is_unhandled) {
        SPDLOG_ERROR("Unknown file type extension \"{}\"", filename.filename().string());

        std::string error = "Unable to read the \"" + extension + "\" extension.\n\n";
        error += "Want to help? Check out our GitHub page for information on how to contribute.";
        Window::Get()->ShowMessageBox(error);
    }
}

void FileLoader::ReadTexture(const std::filesystem::path& filename, ReadFileResultCallback callback)
{
    using namespace ava::AvalancheTexture;

    FileLoader::Get()->ReadFile(
        filename,
        [this, filename, callback](bool success, std::vector<uint8_t> data) {
            if (!success) {
                // ddsc failed. look for the source file instead
                if (filename.extension() == ".ddsc") {
                    auto source_filename = filename;
                    source_filename.replace_extension(g_IsJC4Mode ? ".atx1" : ".hmddsc"); // TODO: ATX2

                    // we should look for .atx2 first, if that doesn't exist, look for .atx1

                    auto cb = [&, callback](bool success, std::vector<uint8_t> data) {
                        if (success) {
                            std::vector<uint8_t> out_buffer;
                            ParseTextureSource(&data, &out_buffer);
                            return callback(success, std::move(out_buffer));
                        }

                        return callback(success, {});
                    };

                    if (FileLoader::UseBatches) {
                        FileLoader::Get()->ReadFileBatched(source_filename, cb);
                    } else {
                        FileLoader::Get()->ReadFile(source_filename, cb, ReadFileFlags_SkipTextureLoader);
                    }

                    return;
                }
                return callback(false, {});
            }

            // raw DDS handler
            if (filename.extension() == ".dds") {
                return callback(true, std::move(data));
            }

            // raw source handler
            if (filename.extension() == ".hmddsc" || filename.extension() == ".atx1"
                || filename.extension() == ".atx2") {
                std::vector<uint8_t> source_buffer;
                ParseTextureSource(&data, &source_buffer);
                return callback(true, std::move(source_buffer));
            }

            // figure out which source file to load
            uint8_t source = 0;
            if (g_IsJC4Mode) {
                auto atx2 = filename;
                atx2.replace_extension(".atx2");

                // TODO: .atx2 is very weird.
                // in the archive they as usually offset 0, size 0 - figure this out before we enable them again.

                bool has_atx2 =
                    false; // FileLoader::Get()->HasFileInDictionary(ava::hashlittle(atx2.string().c_str()));
                if (!has_atx2) {
                    auto atx1 = filename;
                    atx1.replace_extension(".atx1");

                    bool has_atx1 = FileLoader::Get()->HasFileInDictionary(ava::hashlittle(atx1.string().c_str()));
                    source        = static_cast<uint8_t>(has_atx1);
                } else {
                    source = 2;
                }
            } else {
                auto hmddsc = filename;
                hmddsc.replace_extension(".hmddsc");
                if (FileLoader::Get()->HasFileInDictionary(ava::hashlittle(hmddsc.string().c_str()))) {
                    source = 1;
                }
            }

            byte_array_buffer buf(data);
            std::istream      stream(&buf);

            // read header
            AvtxHeader header{};
            stream.read((char*)&header, sizeof(AvtxHeader));
            if (header.m_Magic != AVTX_MAGIC) {
                SPDLOG_ERROR("Invalid AVTX header magic!");
                return callback(false, {});
            }

            if (header.m_Version != 1) {
                SPDLOG_ERROR("Invalid AVTX version!");
                return callback(false, {});
            }

            // find the best stream to use
            const uint8_t     stream_index = FindBestStream(header, source);
            const uint32_t    rank         = GetRank(header, stream_index);
            const AvtxStream& avtx_stream  = header.m_Streams[stream_index];

            // generate the DDS header
            DDS_HEADER dds_header{};
            dds_header.size        = sizeof(DDS_HEADER);
            dds_header.flags       = (DDS_TEXTURE | DDSD_MIPMAPCOUNT);
            dds_header.width       = (header.m_Width >> rank);
            dds_header.height      = (header.m_Height >> rank);
            dds_header.depth       = (header.m_Depth >> rank);
            dds_header.mipMapCount = 1;
            dds_header.ddspf       = TextureManager::GetPixelFormat((DXGI_FORMAT)header.m_Format);
            dds_header.caps        = (DDSCAPS_COMPLEX | DDSCAPS_TEXTURE);

            //
            bool isDXT10 = (dds_header.ddspf.fourCC == MAKEFOURCC('D', 'X', '1', '0'));

            // resize the final output buffer
            std::vector<uint8_t> dds_buffer;
            dds_buffer.resize(sizeof(DDS_MAGIC) + sizeof(DDS_HEADER) + (isDXT10 ? sizeof(DDS_HEADER_DXT10) : 0));

            //
            byte_vector_writer w_buf(&dds_buffer);
            w_buf.setp(0);

            // write the DDS header
            w_buf.write((char*)&DDS_MAGIC, sizeof(uint32_t));
            w_buf.write((char*)&dds_header, sizeof(DDS_HEADER));

            // write the DXT10 header
            if (isDXT10) {
                DDS_HEADER_DXT10 dxt10header{};
                dxt10header.dxgiFormat        = (DXGI_FORMAT)header.m_Format;
                dxt10header.resourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE2D;
                dxt10header.arraySize         = 1;

                w_buf.write((char*)&dxt10header, sizeof(DDS_HEADER_DXT10));
            }

            const uint32_t stream_offset = avtx_stream.m_Offset;
            const uint32_t stream_size   = avtx_stream.m_Size;

            // if we need to load the source file, request it
            if (source > 0) {
                auto source_filename = filename;

                if (g_IsJC4Mode) {
                    std::string source_extension = ".atx" + std::to_string(source);
                    source_filename.replace_extension(source_extension);
                } else {
                    source_filename.replace_extension(".hmddsc");
                }

                SPDLOG_INFO("Loading texture source: \"{}\".", source_filename.string());

                // read file callback
                auto cb = [dds_buffer, stream_offset, stream_size, callback](bool                 success,
                                                                             std::vector<uint8_t> data) mutable {
                    if (success) {
                        byte_vector_writer w_buf(&dds_buffer);
                        w_buf.write((char*)data.data() + stream_offset, stream_size);
                        return callback(true, std::move(dds_buffer));
                    }

                    return callback(false, {});
                };

                if (FileLoader::UseBatches) {
                    FileLoader::Get()->ReadFileBatched(source_filename, cb);
                } else {
                    FileLoader::Get()->ReadFile(source_filename, cb, ReadFileFlags_SkipTextureLoader);
                }

                return;
            }

            // write the texture data
            w_buf.write((char*)data.data() + stream_offset, stream_size);
            return callback(true, std::move(dds_buffer));
        },
        ReadFileFlags_SkipTextureLoader);
}

bool FileLoader::ReadAVTX(const std::vector<uint8_t>& buffer, std::vector<uint8_t>* out_buffer)
{
    using namespace ava::AvalancheTexture;

    try {
        TextureEntry         entry{};
        std::vector<uint8_t> texture_buffer;
        ava::AvalancheTexture::ReadBestEntry(buffer, &entry, &texture_buffer, {});

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
        out_buffer->resize(sizeof(DDS_MAGIC) + sizeof(DDS_HEADER) + (isDXT10 ? sizeof(DDS_HEADER_DXT10) : 0)
                           + texture_buffer.size());

        byte_vector_writer buf(out_buffer);
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
        buf.write((char*)texture_buffer.data(), texture_buffer.size());
        return true;
    } catch (const std::exception& e) {
        SPDLOG_ERROR(e.what());
#ifdef _DEBUG
        __debugbreak();
#endif
    }

    return false;
}

void FileLoader::ParseTextureSource(std::vector<uint8_t>* buffer, std::vector<uint8_t>* out_buffer)
{
    assert(buffer && buffer->size() > 0);

    DDS_HEADER dds_header{};
    dds_header.size        = sizeof(DDS_HEADER);
    dds_header.flags       = (DDS_TEXTURE | DDSD_MIPMAPCOUNT);
    dds_header.width       = 512;
    dds_header.height      = 512;
    dds_header.depth       = 1;
    dds_header.mipMapCount = 1;
    dds_header.ddspf       = TextureManager::GetPixelFormat(DXGI_FORMAT_BC1_UNORM);
    dds_header.caps        = (DDSCAPS_COMPLEX | DDSCAPS_TEXTURE);

    out_buffer->resize(sizeof(uint32_t) + sizeof(DDS_HEADER) + buffer->size());

    byte_vector_writer buf(out_buffer);
    buf.setp(0);

    buf.write((char*)&DDS_MAGIC, sizeof(uint32_t));
    buf.write((char*)&dds_header, sizeof(DDS_HEADER));
    buf.write((char*)buffer->data(), buffer->size());
}

bool FileLoader::WriteAVTX(Texture* texture, std::vector<uint8_t>* out_buffer)
{
    if (!texture || !texture->IsLoaded()) {
        return false;
    }

    using namespace ava::AvalancheTexture;

    AvtxHeader header;

    auto& tex_buffer = texture->GetBuffer();
    auto& tex_desc   = texture->GetDesc();

    // calculate the offset to skip
    const auto dds_header_size = (sizeof(DDS_MAGIC) + sizeof(DDS_HEADER));

    header.m_Dimension    = 2;
    header.m_Format       = tex_desc.Format;
    header.m_Width        = tex_desc.Width;
    header.m_Height       = tex_desc.Height;
    header.m_Depth        = 1;
    header.m_Flags        = 0;
    header.m_Mips         = tex_desc.MipLevels;
    header.m_MipsRedisent = tex_desc.MipLevels;

    // set the stream
    header.m_Streams[0].m_Offset    = sizeof(header);
    header.m_Streams[0].m_Size      = (tex_buffer.size() - dds_header_size);
    header.m_Streams[0].m_Alignment = 16;
    header.m_Streams[0].m_TileMode  = false;
    header.m_Streams[0].m_Source    = false;

    out_buffer->resize(sizeof(AvtxHeader) + header.m_Streams[0].m_Size);
    byte_vector_writer buf(out_buffer);
    buf.setp(0);

    buf.write((char*)&header, sizeof(AvtxHeader));
    buf.write((char*)tex_buffer.data() + dds_header_size, header.m_Streams[0].m_Size);
    return true;
}

std::tuple<AvalancheArchive*, ava::StreamArchive::ArchiveEntry>
FileLoader::GetStreamArchiveFromFile(const std::filesystem::path& file, AvalancheArchive* archive)
{
    if (archive) {
        for (const auto& entry : archive->GetEntries()) {
            if (entry.m_Filename == file || entry.m_Filename.find(file.string()) != std::string::npos) {
                return {archive, entry};
            }
        }
    } else {
        std::lock_guard<std::recursive_mutex> _lk{AvalancheArchive::InstancesMutex};
        for (const auto& arc : AvalancheArchive::Instances) {
            for (const auto& entry : arc.second->GetEntries()) {
                if (entry.m_Filename == file || entry.m_Filename.find(file.string()) != std::string::npos) {
                    return {arc.second.get(), entry};
                }
            }
        }
    }

    return {nullptr, ava::StreamArchive::ArchiveEntry{}};
}

bool FileLoader::ReadArchiveTable(const std::filesystem::path&                        filename,
                                  std::vector<ava::ArchiveTable::TabEntry>*           output,
                                  std::vector<ava::ArchiveTable::TabCompressedBlock>* output_blocks)
{
    std::ifstream stream(filename, std::ios::binary);
    if (stream.fail()) {
        SPDLOG_ERROR("Failed to open file stream!");
        return false;
    }

    const size_t         size = std::filesystem::file_size(filename);
    std::vector<uint8_t> buffer(size);
    stream.read((char*)buffer.data(), size);
    stream.close();

    try {
        if (g_IsJC4Mode) {
            ava::ArchiveTable::Parse(buffer, output, output_blocks);
        } else {
            std::vector<ava::legacy::ArchiveTable::TabEntry> entries;
            ava::legacy::ArchiveTable::Parse(buffer, &entries);

            // legacy TabEntry convert
            output->reserve(entries.size());
            for (const auto& entry : entries) {
                ava::ArchiveTable::TabEntry _entry{};
                _entry.m_NameHash         = entry.m_NameHash;
                _entry.m_Offset           = entry.m_Offset;
                _entry.m_Size             = entry.m_Size;
                _entry.m_UncompressedSize = entry.m_Size;
                output->push_back(std::move(_entry));
            }
        }

        return true;
    } catch (const std::exception& e) {
        SPDLOG_ERROR(e.what());
#ifdef _DEBUG
        __debugbreak();
#endif
    }

    return false;
}

bool FileLoader::ReadArchiveTableEntry(const std::filesystem::path& table, const std::filesystem::path& filename,
                                       ava::ArchiveTable::TabEntry*                        output,
                                       std::vector<ava::ArchiveTable::TabCompressedBlock>* output_blocks)
{
    auto namehash = ava::hashlittle(filename.generic_string().c_str());
    return ReadArchiveTableEntry(table, namehash, output, output_blocks);
}

bool FileLoader::ReadArchiveTableEntry(const std::filesystem::path& table, uint32_t name_hash,
                                       ava::ArchiveTable::TabEntry*                        output,
                                       std::vector<ava::ArchiveTable::TabCompressedBlock>* output_blocks)
{
    std::vector<ava::ArchiveTable::TabEntry> entries;
    if (ReadArchiveTable(table, &entries, output_blocks)) {
        auto it = std::find_if(entries.begin(), entries.end(),
                               [&](const ava::ArchiveTable::TabEntry& entry) { return entry.m_NameHash == name_hash; });

        if (it != entries.end()) {
            *output = (*it);
            return true;
        }
    }

    return false;
}

bool FileLoader::ReadFileFromArchive(const std::string& directory, const std::string& archive, uint32_t namehash,
                                     std::vector<uint8_t>* out_buffer)
{
    std::filesystem::path jc_directory = Window::Get()->GetJustCauseDirectory();
    const auto            tab_file     = jc_directory / directory / (archive + ".tab");
    const auto            arc_file     = jc_directory / directory / (archive + ".arc");

    // ensure the tab file exists
    if (!std::filesystem::exists(tab_file)) {
        SPDLOG_ERROR("Can't find TAB file (\"{}\")", tab_file.string());
        return false;
    }

    // ensure the arc file exists
    if (!std::filesystem::exists(arc_file)) {
        SPDLOG_ERROR("Can't find ARC file (\"{}\")", arc_file.string());
        return false;
    }

    using namespace ava;

    ArchiveTable::TabEntry                        entry{};
    std::vector<ArchiveTable::TabCompressedBlock> compression_blocks;
    if (ReadArchiveTableEntry(tab_file, namehash, &entry, &compression_blocks)) {
        SPDLOG_INFO("NameHash={}, Offset={}, Size={}, UncompressedSize={}, Library={}, "
                    "CompressedBlockIndex={}",
                    namehash, entry.m_Offset, entry.m_Size, entry.m_UncompressedSize, entry.m_Library,
                    entry.m_CompressedBlockIndex);

        // read the arc file
        std::ifstream stream(arc_file, std::ios::ate | std::ios::binary);
        if (stream.fail()) {
            SPDLOG_ERROR("Failed to open file stream!");
            return false;
        }

        g_ArchiveLoadCount++;
        SPDLOG_INFO("Current archive load count: {}", g_ArchiveLoadCount);

        stream.seekg(entry.m_Offset);

        // file is compressed!
        if (entry.m_Size != entry.m_UncompressedSize && entry.m_Library != ArchiveTable::E_COMPRESS_LIBRARY_NONE) {
            // read the compressed buffer
            std::vector<uint8_t> compressed_buffer(ArchiveTable::GetEntryRequiredBufferSize(entry, compression_blocks));
            stream.read((char*)compressed_buffer.data(), compressed_buffer.size());

            assert(entry.m_Size != entry.m_UncompressedSize);

            // decompress the buffer
            out_buffer->resize(entry.m_UncompressedSize);
            ArchiveTable::DecompressEntryBuffer(compressed_buffer, entry, out_buffer, compression_blocks);
            return !out_buffer->empty();
        }

        out_buffer->resize(entry.m_Size);
        stream.read((char*)out_buffer->data(), entry.m_Size);
        return !out_buffer->empty();
    }

    return false;
}

bool FileLoader::HasFileInDictionary(uint32_t name_hash)
{
    return m_Dictionary.find(name_hash) != m_Dictionary.end();
}

DictionaryLookupResult FileLoader::LocateFileInDictionary(const std::filesystem::path& filename)
{
    return LocateFileInDictionary(ava::hashlittle(filename.generic_string().c_str()));
}

DictionaryLookupResult FileLoader::LocateFileInDictionary(uint32_t name_hash)
{
    auto it = m_Dictionary.find(name_hash);
    if (it == m_Dictionary.end()) {
        return {};
    }

    const auto& file = (*it);
    const auto& path = file.second.second.at(0);
    const auto  pos  = path.find_last_of("/");

    return {path.substr(0, pos), path.substr(pos + 1, path.length()), file.first};
}

DictionaryLookupResult FileLoader::LocateFileInDictionaryByPartOfName(const std::filesystem::path& filename)
{
    const auto& _filename = filename.generic_string();

    auto it =
        std::find_if(m_Dictionary.begin(), m_Dictionary.end(),
                     [&](const std::pair<uint32_t, std::pair<std::filesystem::path, std::vector<std::string>>>& item) {
                         const auto& item_fn = item.second.first;
                         return item_fn == _filename
                                || (item_fn.generic_string().find(_filename) != std::string::npos
                                    && item_fn.extension() == filename.extension());
                     });

    if (it == m_Dictionary.end()) {
        return {};
    }

    const auto& file = (*it);
    const auto& path = file.second.second.at(0);
    const auto  pos  = path.find_last_of("/");

    return {path.substr(0, pos), path.substr(pos + 1, path.length()), file.first};
}

void FileLoader::RegisterReadCallback(const std::vector<std::string>& extensions, FileReadHandler fn)
{
    for (const auto& extension : extensions) {
        m_FileReadHandlers[extension].emplace_back(fn);
    }
}

void FileLoader::RegisterSaveCallback(const std::vector<std::string>& extensions, FileSaveHandler fn)
{
    for (const auto& extension : extensions) {
        m_FileSaveHandlers[extension].emplace_back(fn);
    }
}

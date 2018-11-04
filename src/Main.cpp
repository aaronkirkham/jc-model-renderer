#include <Window.h>
#include <Input.h>
#include <Settings.h>
#include <graphics/Renderer.h>
#include <graphics/UI.h>

#include <vector>
#include <shellapi.h>
#include <DbgHelp.h>

#include <json.hpp>
#include <httplib.h>

#include <jc3/FileLoader.h>
#include <jc3/formats/RenderBlockModel.h>
#include <jc3/formats/AvalancheArchive.h>
#include <jc3/formats/RuntimeContainer.h>

#include <import_export/ImportExportManager.h>
#include <import_export/wavefront_obj.h>
#include <import_export/DDSC.h>
#include <import_export/AvalancheArchive.h>

extern fs::path g_JC3Directory = "";
extern bool g_DrawBoundingBoxes = true;
extern bool g_ShowModelLabels = true;

void CheckForUpdates(bool show_no_update_messagebox)
{
    static int32_t current_version[3] = { VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION };

    std::thread([show_no_update_messagebox] {
        try {
            httplib::Client client("kirkh.am");
            auto res = client.get("/jc3-rbm-renderer/latest.json");
            if (res && res->status == 200) {
                auto data = json::parse(res->body);
                auto version_str = data["version"].get<std::string>();

                int32_t latest_version[3] = { 0 };
                std::sscanf(version_str.c_str(), "%d.%d.%d", &latest_version[0], &latest_version[1], &latest_version[2]);

                if (std::lexicographical_compare(current_version, current_version + 3, latest_version, latest_version + 3)) {
                    std::stringstream msg;
                    msg << "A new version (" << version_str << ") is available for download." << std::endl << std::endl;
                    msg << "Do you want to go to the release page on GitHub?";

                    if (Window::Get()->ShowMessageBox(msg.str(), MB_ICONQUESTION | MB_YESNO) == IDYES) {
                        ShellExecuteA(nullptr, "open", "https://github.com/aaronkirkham/jc3-rbm-renderer/releases/latest", nullptr, nullptr, SW_SHOWNORMAL);
                    }
                }
                else if (show_no_update_messagebox) {
                    Window::Get()->ShowMessageBox("You have the latest version!", MB_ICONINFORMATION);
                }
            }
        }
        catch (...) {
        }
    }).detach();
}

void SelectJustCause3Directory()
{
    // ask the user to select their jc3 directory, we can't find it!
    Window::Get()->ShowFolderSelection("Please select your Just Cause 3 install folder.", [&](const fs::path& selected) {
        Settings::Get()->SetValue("jc3_directory", selected.string());
        g_JC3Directory = selected;
    }, [] {
        Window::Get()->ShowMessageBox("Unable to find Just Cause 3 root directory.\n\nSome features will be disabled.");
    });
}

#ifndef DEBUG
LONG WINAPI UnhandledExceptionHandler(struct _EXCEPTION_POINTERS* exception_pointers);
#endif

#include <import_export/ImportExportManager.h>
#include <jc3/RenderBlockFactory.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR psCmdLine, int32_t iCmdShow)
{
#ifndef DEBUG
    // generate minidumps
    SetUnhandledExceptionFilter(UnhandledExceptionHandler);
#endif

    static auto ReadRegistryKeyString = [](HKEY location, const char* subkey, const char* key, char* data, DWORD size) {
        HKEY hKey = nullptr;
        if (RegOpenKeyExA(location, subkey, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD type = REG_SZ;
            auto status = RegQueryValueExA(hKey, key, 0, &type, (uint8_t *)data, &size);
            RegCloseKey(hKey);

            return (status == ERROR_SUCCESS);
        }

        return false;
    };

    g_JC3Directory = Settings::Get()->GetValue<std::string>("jc3_directory");
    g_DrawBoundingBoxes = Settings::Get()->GetValue<bool>("draw_bounding_boxes", true);
    g_ShowModelLabels = Settings::Get()->GetValue<bool>("show_model_labels", true);

    DEBUG_LOG("JC3 Directory: " << g_JC3Directory);

    // TODO: Validate the jc3 directory, make sure we can see some common stuff like JustCause3.exe / archives_win64 folder.

    // is the directory invalid?
    if (g_JC3Directory.empty() || !fs::exists(g_JC3Directory)) {
        // try find the install directory via the steam install folder
        char steam_path[MAX_PATH] = { 0 };
        if (ReadRegistryKeyString(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam", "SteamPath", steam_path, MAX_PATH)) {
            g_JC3Directory = fs::path(steam_path) / "steamapps" / "common" / "Just Cause 3";

            // TODO: Parse C:/Program Files (x86)/Steam/steamapps/libraryfolders.vdf to check all library folders for the game location

            // if the directory exists, save the settings now
            if (fs::exists(g_JC3Directory)) {
                Settings::Get()->SetValue("jc3_directory", g_JC3Directory.string());
            }
        }
    }

    if (Window::Get()->Initialise(hInstance)) {
        // do we need to select the install directory manually?
        if (g_JC3Directory.empty() || !fs::exists(g_JC3Directory)) {
            SelectJustCause3Directory();
        }

#ifndef DEBUG
        // check for any updates
        CheckForUpdates();
#endif

#ifdef DEBUG
        Input::Get()->Events().KeyUp.connect([](uint32_t key) {
            if (key == VK_F1) {
                //FileLoader::Get()->LocateFileInDictionary("models/jc_environments/props/animation_prop/textures/bucket_dif.hmddsc");
                FileLoader::Get()->LocateFileInDictionary("bucket_dif.hmddsc");
            }
            else if (key == VK_F2) {
                fs::path filename = "editor/entities/gameobjects/main_character.ee";
                FileLoader::Get()->ReadFile(filename, [&, filename](bool success, FileBuffer data) {
                    if (success) {
                        std::thread([&, filename, data] {
                            auto archive = AvalancheArchive::make(filename, data);

                            while (!archive->GetStreamArchive())
                                std::this_thread::sleep_for(std::chrono::milliseconds(50));

                            fs::path rbm = "models/jc_characters/main_characters/rico/rico_body_lod1.rbm";
                            RenderBlockModel::Load(rbm);
                        }).detach();
                    }
                });
            }
            else if (key == VK_F3) {
                fs::path filename = "editor/entities/jc_weapons/02_two_handed/w141_rpg_uvk_13/w141_rpg_uvk_13.ee";
                FileLoader::Get()->ReadFile(filename, [&, filename](bool success, FileBuffer data) {
                    if (success) {
                        std::thread([&, filename, data] {
                            auto archive = AvalancheArchive::make(filename, data);

                            while (!archive->GetStreamArchive())
                                std::this_thread::sleep_for(std::chrono::milliseconds(50));

                            fs::path rbm = "models/jc_weapons/02_two_handed/w141_rpg_uvk_13/w141_rpg_uvk_13_base_body_lod1.rbm";
                            RenderBlockModel::Load(rbm);
                        }).detach();
                    }
                });
            }
            else if (key == VK_F4) {
                fs::path filename = "editor/entities/jc_vehicles/01_land/v0405_car_mugello_moderncircuitracer/v0405_car_mugello_moderncircuitracer_civilian_01.ee";
                FileLoader::Get()->ReadFile(filename, [&, filename](bool success, FileBuffer data) {
                    if (success) {
                        AvalancheArchive::make(filename, data);

                        fs::path rbm = "models/jc_vehicles/01_land/v0405_car_mugello_moderncircuitracer/moderncircuitracer_body_lod1.rbm";
                        RenderBlockModel::Load(rbm);
                    }
                });
            }
            else if (key == VK_F5) {
                fs::path filename = "editor/entities/jc_vehicles/02_air/v4602_plane_urga_fighterbomber/v4602_plane_urga_fighterbomber_debug.ee";
                FileLoader::Get()->ReadFile(filename, [&, filename](bool success, FileBuffer data) {
                    if (success) {
                        std::thread([&, filename, data] {
                            auto archive = AvalancheArchive::make(filename, data);

                            while (!archive->GetStreamArchive())
                                std::this_thread::sleep_for(std::chrono::milliseconds(50));

                            fs::path epe = "editor/entities/jc_vehicles/02_air/v4602_plane_urga_fighterbomber/v4602_plane_urga_fighterbomber_debug.epe";
                            auto rc = RuntimeContainer::get(epe.string());

                            if (rc) RenderBlockModel::LoadFromRuntimeContainer(epe, rc);
                            else {
                                RuntimeContainer::Load(epe, [&, epe](std::shared_ptr<RuntimeContainer> rc) {
                                    RenderBlockModel::LoadFromRuntimeContainer(epe, rc);
                                });
                            }
 
                        }).detach();
                    }
                });
            }
            else if (key == VK_F6) {
                fs::path filename = "editor/entities/jc_vehicles/01_land/v0803_car_na_monstertruck/v0803_car_na_monstertruck_civilian_01.ee";
                FileLoader::Get()->ReadFile(filename, [&, filename](bool success, FileBuffer data) {
                    if (success) {
                        std::thread([&, filename, data] {
                            auto archive = AvalancheArchive::make(filename, data);

                            while (!archive->GetStreamArchive())
                                std::this_thread::sleep_for(std::chrono::milliseconds(50));

                            fs::path epe = "editor/entities/jc_vehicles/01_land/v0803_car_na_monstertruck/v0803_car_na_monstertruck_civilian_01.epe";
                            auto rc = RuntimeContainer::get(epe.string());

                            if (rc) RenderBlockModel::LoadFromRuntimeContainer(epe, rc);
                            else {
                                RuntimeContainer::Load(epe, [&, epe](std::shared_ptr<RuntimeContainer> rc) {
                                    RenderBlockModel::LoadFromRuntimeContainer(epe, rc);
                                });
                            }

                        }).detach();
                    }
                });
            }
            else if (key == VK_F7) {
                /*fs::path filename = "editor/entities/gameobjects/main_character.epe";
                RuntimeContainer::Load(filename, [&](std::shared_ptr<RuntimeContainer> rc) {
                    FileLoader::Get()->WriteRuntimeContainer(rc.get());
                });*/

                const auto& importers = ImportExportManager::Get()->GetImporters(".rbm");
                importers[0]->Import("C:/users/aaron/desktop/nanos cube/nanos.obj", [&](bool success, std::any data) {
                    const auto rbm = RenderBlockModel::make("nanos.obj");
                    const auto render_block = RenderBlockFactory::CreateRenderBlock("GeneralMkIII");

                    rbm->GetRenderBlocks().emplace_back(render_block);

                    auto & [vertices, uvs, normals, indices] =
                        std::any_cast<std::tuple<floats_t, floats_t, floats_t, uint16s_t>>(data);

                    render_block->SetData(&vertices, &indices, &uvs);
                    render_block->Create();

                    Renderer::Get()->AddToRenderList(render_block);
                });
            }
        });
#endif

        // register read file type callbacks
        FileLoader::Get()->RegisterReadCallback({ ".rbm" }, RenderBlockModel::ReadFileCallback);
        FileLoader::Get()->RegisterReadCallback({ ".ee", ".bl", ".nl", ".fl" }, AvalancheArchive::ReadFileCallback);
        FileLoader::Get()->RegisterReadCallback({ ".epe", ".blo" }, RuntimeContainer::ReadFileCallback);
        FileLoader::Get()->RegisterReadCallback({ ".dds", ".ddsc", ".hmddsc" }, [&](const fs::path& filename, FileBuffer data, bool external) {
            // parse the compressed texture if the file was loaded from an external source
            if (external) {
                if (filename.extension() == ".ddsc") {
                    FileBuffer out;
                    if (FileLoader::Get()->ParseCompressedTexture(&data, &out)) {
                        TextureManager::Get()->GetTexture(filename, &out, (TextureManager::CREATE_IF_NOT_EXISTS | TextureManager::IS_UI_RENDERABLE));
                        return true;
                    }
                }
                else if (filename.extension() == ".hmddsc") {
                    FileBuffer out;
                    FileLoader::Get()->ParseHMDDSCTexture(&data, &out);
                    TextureManager::Get()->GetTexture(filename, &out, (TextureManager::CREATE_IF_NOT_EXISTS | TextureManager::IS_UI_RENDERABLE));
                    return true;
                }
            }

            TextureManager::Get()->GetTexture(filename, &data, (TextureManager::CREATE_IF_NOT_EXISTS | TextureManager::IS_UI_RENDERABLE));
            return true;
        });

        // register save file type callbacks
        FileLoader::Get()->RegisterSaveCallback({ ".rbm" }, RenderBlockModel::SaveFileCallback);
        FileLoader::Get()->RegisterSaveCallback({ ".ee", ".bl", ".nl", ".fl" }, AvalancheArchive::SaveFileCallback);

        // register file type context menu callbacks
        UI::Get()->RegisterContextMenuCallback({ ".rbm" }, RenderBlockModel::ContextMenuUI);
        UI::Get()->RegisterContextMenuCallback({ ".epe" }, RuntimeContainer::ContextMenuUI);

        // register importers and exporters
        ImportExportManager::Get()->Register(new import_export::Wavefront_Obj);
        ImportExportManager::Get()->Register(new import_export::DDSC);
        ImportExportManager::Get()->Register(new import_export::AvalancheArchive);

        //
        Renderer::Get()->Events().PostRender.connect([&](RenderContext_t* context) {
            //std::lock_guard<std::recursive_mutex> _lk{ RenderBlockModel::InstancesMutex };

            ImGui::SetNextWindowBgAlpha(0.0f);
            ImGui::Begin("Model Manager", nullptr, (ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings));
            {
                const auto& window_size = Window::Get()->GetSize();

                uint32_t vertices = 0;
                uint32_t indices = 0;
                uint32_t triangles = 0;

                for (const auto& model : RenderBlockModel::Instances) {
                    for (const auto& block : model.second->GetRenderBlocks()) {
                        if (!block->IsVisible() || !block->GetVertexBuffer() || !block->GetIndexBuffer()) {
                            continue;
                        }

                        auto index_count = block->GetIndexBuffer()->m_ElementCount;

                        vertices += block->GetVertexBuffer()->m_ElementCount;
                        indices += index_count;
                        triangles += (index_count / 3);
                    }
                }

                ImGui::SetWindowPos({ 10, (window_size.y - 35) });
                ImGui::Text("Models: %d, Vertices: %d, Indices: %d, Triangles: %d, Textures: %d, Shaders: %d", RenderBlockModel::Instances.size(), vertices, indices, triangles, TextureManager::Get()->GetCacheSize(), ShaderManager::Get()->GetCacheSize());
            }
            ImGui::End();

            // draw gizmos
            for (const auto& model : RenderBlockModel::Instances) {
                model.second->DrawGizmos();
            }
        });

#if 0
        auto adf = FileLoader::Get()->ReadAdf("D:/Steam/steamapps/common/Just Cause 3/Shaders_F.shader_bundle");
#endif

        // run!
        Window::Get()->Run();
    }

    return 0;
}

#ifndef DEBUG
typedef BOOL(WINAPI *MINIDUMPWRITEDUMP)(
    HANDLE process,
    uint32_t process_id,
    HANDLE file,
    MINIDUMP_TYPE type,
    const PMINIDUMP_EXCEPTION_INFORMATION exception_param,
    const PMINIDUMP_USER_STREAM_INFORMATION user_stream,
    const PMINIDUMP_CALLBACK_INFORMATION callback);

LONG WINAPI UnhandledExceptionHandler(struct _EXCEPTION_POINTERS* exception_pointers)
{
    auto dbghelp = LoadLibrary("dbghelp.dll");
    auto WriteDump = (MINIDUMPWRITEDUMP)GetProcAddress(dbghelp, "MiniDumpWriteDump");

    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    auto time = std::put_time(&tm, "%d-%m-%Y-%H-%M-%S");
    
    std::stringstream filename;
    filename << fs::current_path();
    filename << "\\" << VER_PRODUCTNAME_STR << "-" << VER_FILE_VERSION_STR << "-" << time << ".dmp";

    auto file = CreateFile(filename.str().c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

    _MINIDUMP_EXCEPTION_INFORMATION except_info;
    except_info.ThreadId = GetCurrentThreadId();
    except_info.ExceptionPointers = exception_pointers;
    except_info.ClientPointers = FALSE;

    const auto process = GetCurrentProcess();
    const auto process_id = GetCurrentProcessId();

    WriteDump(process, process_id, file, MiniDumpNormal, &except_info, nullptr, nullptr);
    CloseHandle(file);

    std::stringstream msg;
    msg << "Something somewhere somehow went wrong.\n\n";
    msg << "A dump file was created at " << filename.str() << "\n\n";
    msg << "Please create a bug report on our GitHub and attach the dump file and we'll try and fix it.";

    MessageBox(nullptr, msg.str().c_str(), nullptr, MB_ICONSTOP | MB_OK);
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif
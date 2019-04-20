#include <Windows.h>

#include <DbgHelp.h>
#include <shellapi.h>

#include "settings.h"
#include "version.h"
#include "window.h"

#include "graphics/renderer.h"
#include "graphics/texture_manager.h"

#include "game/file_loader.h"
#include "game/formats/avalanche_archive.h"
#include "game/formats/render_block_model.h"
#include "game/formats/runtime_container.h"
#include "game/render_block_factory.h"
#include "game/renderblocks/irenderblock.h"

#include "import_export/avalanche_archive.h"
#include "import_export/ddsc.h"
#include "import_export/import_export_manager.h"
#include "import_export/wavefront_obj.h"

extern bool g_IsJC4Mode         = false;
extern bool g_DrawBoundingBoxes = true;
extern bool g_ShowModelLabels   = true;

#ifndef DEBUG
LONG WINAPI UnhandledExceptionHandler(struct _EXCEPTION_POINTERS* exception_pointers);
#endif

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR psCmdLine, int32_t iCmdShow)
{
#ifndef DEBUG
    // generate minidumps
    SetUnhandledExceptionFilter(UnhandledExceptionHandler);
#endif

    static auto ReadRegistryKeyString = [](HKEY location, const char* subkey, const char* key, char* data, DWORD size) {
        HKEY hKey = nullptr;
        if (RegOpenKeyExA(location, subkey, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD type   = REG_SZ;
            auto  status = RegQueryValueExA(hKey, key, 0, &type, (uint8_t*)data, &size);
            RegCloseKey(hKey);

            return (status == ERROR_SUCCESS);
        }

        return false;
    };

    g_IsJC4Mode         = Settings::Get()->GetValue<bool>("jc4_mode", false);
    g_DrawBoundingBoxes = Settings::Get()->GetValue<bool>("draw_bounding_boxes", false);
    g_ShowModelLabels   = Settings::Get()->GetValue<bool>("show_model_labels", false);

    // TODO: Validate the jc3 directory, make sure we can see some common stuff like JustCause3.exe / archives_win64
    // folder.

#if 0
    // try find the jc3 directory from the steam installation folder
    std::filesystem::path jc3_directory = Settings::Get()->GetValue<std::string>("jc3_directory");
    if (jc3_directory.empty() || !std::filesystem::exists(jc3_directory)) {
        char steam_path[MAX_PATH] = {0};
        if (ReadRegistryKeyString(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam", "SteamPath", steam_path, MAX_PATH)) {
            jc3_directory = std::filesystem::path(steam_path) / "steamapps" / "common" / "Just Cause 3";

            // TODO: Parse C:/Program Files (x86)/Steam/steamapps/libraryfolders.vdf to check all library folders for
            // the game location

            // if the directory exists, save the settings now
            if (std::filesystem::exists(jc3_directory)) {
                Settings::Get()->SetValue("jc3_directory", jc3_directory.string());
            }
        }
    }
#endif

    const auto& jc_directory = Window::Get()->GetJustCauseDirectory();
    if (Window::Get()->Initialise(hInstance)) {
        LOG_INFO("Just Cause directory: \"{}\"", jc_directory.string());

        // do we need to select the install directory manually?
        if (jc_directory.empty() || !std::filesystem::exists(jc_directory)) {
            Window::Get()->SelectJustCauseDirectory();
        }

#ifndef DEBUG
        // check for any updates
        Window::Get()->CheckForUpdates();
#endif

#if 1
        // input thread
        std::thread([&] {
            while (true) {
                if (!Window::Get()->HasFocus()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    continue;
                }

                if (GetAsyncKeyState(VK_F1) & 1) {
                    std::filesystem::path filename = "editor/entities/characters/main_characters/sheldon.ee";
                    FileLoader::Get()->ReadFile(filename, [&, filename](bool success, FileBuffer data) {
                        auto archive = AvalancheArchive::make(filename, data);
                        while (!archive->GetStreamArchive()) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        }

                        std::filesystem::path rbm = "models/characters/main_characters/sheldon/sheldon_body_002.modelc";
                        RenderBlockModel::Load(rbm);
                    });
                }

                if (GetAsyncKeyState(VK_F2) & 1) {
                    std::filesystem::path filename = "editor/entities/gameobjects/main_character.ee";
                    FileLoader::Get()->ReadFile(filename, [&, filename](bool success, FileBuffer data) {
                        if (success) {
                            auto archive = AvalancheArchive::make(filename, data);
                            while (!archive->GetStreamArchive()) {
                                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                            }

                            std::filesystem::path rbm = "models/jc_characters/main_characters/rico/rico_body_lod1.rbm";
                            RenderBlockModel::Load(rbm);
                        }
                    });
                }

                if (GetAsyncKeyState(VK_F3) & 1) {
                    std::filesystem::path filename =
                        "editor/entities/jc_weapons/02_two_handed/w141_rpg_uvk_13/w141_rpg_uvk_13.ee";
                    FileLoader::Get()->ReadFile(filename, [&, filename](bool success, FileBuffer data) {
                        if (success) {
                            auto archive = AvalancheArchive::make(filename, data);
                            while (!archive->GetStreamArchive()) {
                                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                            }

                            std::filesystem::path rbm = "models/jc_weapons/02_two_handed/w141_rpg_uvk_13/"
                                                        "w141_rpg_uvk_13_base_body_lod1.rbm";
                            RenderBlockModel::Load(rbm);
                        }
                    });
                }

                if (GetAsyncKeyState(VK_F4) & 1) {
                    std::filesystem::path filename =
                        "editor/entities/jc_vehicles/01_land/v0405_car_mugello_moderncircuitracer/"
                        "v0405_car_mugello_moderncircuitracer_civilian_01.ee";
                    FileLoader::Get()->ReadFile(filename, [&, filename](bool success, FileBuffer data) {
                        if (success) {
                            auto archive = AvalancheArchive::make(filename, data);
                            while (!archive->GetStreamArchive()) {
                                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                            }

                            std::filesystem::path rbm =
                                "models/jc_vehicles/01_land/v0405_car_mugello_moderncircuitracer/"
                                "moderncircuitracer_body_lod1.rbm";
                            RenderBlockModel::Load(rbm);
                        }
                    });
                }

                if (GetAsyncKeyState(VK_F5) & 1) {
                    std::filesystem::path filename =
                        "editor/entities/jc_vehicles/02_air/v4602_plane_urga_fighterbomber/"
                        "v4602_plane_urga_fighterbomber_debug.ee";
                    FileLoader::Get()->ReadFile(filename, [&, filename](bool success, FileBuffer data) {
                        if (success) {
                            auto archive = AvalancheArchive::make(filename, data);
                            while (!archive->GetStreamArchive()) {
                                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                            }

                            std::filesystem::path epe =
                                "editor/entities/jc_vehicles/02_air/v4602_plane_urga_fighterbomber/"
                                "v4602_plane_urga_fighterbomber_debug.epe";
                            auto rc = RuntimeContainer::get(epe.string());

                            if (rc) {
                                RenderBlockModel::LoadFromRuntimeContainer(epe, rc);
                            } else {
                                RuntimeContainer::Load(epe, [&, epe](std::shared_ptr<RuntimeContainer> rc) {
                                    RenderBlockModel::LoadFromRuntimeContainer(epe, rc);
                                });
                            }
                        }
                    });
                }

                if (GetAsyncKeyState(VK_F6) & 1) {
                    std::filesystem::path filename = "editor/entities/jc_vehicles/01_land/v0803_car_na_monstertruck/"
                                                     "v0803_car_na_monstertruck_civilian_01.ee";
                    FileLoader::Get()->ReadFile(filename, [&, filename](bool success, FileBuffer data) {
                        if (success) {
                            auto archive = AvalancheArchive::make(filename, data);
                            while (!archive->GetStreamArchive()) {
                                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                            }

                            std::filesystem::path epe = "editor/entities/jc_vehicles/01_land/v0803_car_na_monstertruck/"
                                                        "v0803_car_na_monstertruck_civilian_01.epe";
                            auto rc = RuntimeContainer::get(epe.string());

                            if (rc) {
                                RenderBlockModel::LoadFromRuntimeContainer(epe, rc);
                            } else {
                                RuntimeContainer::Load(epe, [&, epe](std::shared_ptr<RuntimeContainer> rc) {
                                    RenderBlockModel::LoadFromRuntimeContainer(epe, rc);
                                });
                            }
                        }
                    });
                }

                if (GetAsyncKeyState(VK_F7) & 1) {
                    /*std::filesystem::path filename = "editor/entities/gameobjects/main_character.epe";
                    RuntimeContainer::Load(filename, [&](std::shared_ptr<RuntimeContainer> rc) {
                        FileLoader::Get()->WriteRuntimeContainer(rc.get());
                    });*/

                    const auto& importers = ImportExportManager::Get()->GetImporters(".rbm");
                    importers[0]->Import("C:/users/aaron/desktop/nanos cube/nanos.obj",
                                         [&](bool success, std::filesystem::path filename, std::any data) {
                                             const auto rbm = RenderBlockModel::make("nanos.rbm");
                                             const auto render_block =
                                                 RenderBlockFactory::CreateRenderBlock("GeneralMkIII");

                                             rbm->GetRenderBlocks().emplace_back(render_block);

                                             auto& [vertices, indices, materials] =
                                                 std::any_cast<std::tuple<vertices_t, uint16s_t, materials_t>>(data);

                                             render_block->SetData(&vertices, &indices, &materials);
                                             render_block->Create();

                                             Renderer::Get()->AddToRenderList(render_block);
                                         });
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }).detach();
#endif

        // register read file type callbacks
        FileLoader::Get()->RegisterReadCallback({".lod", ".rbm", ".modelc"}, RenderBlockModel::ReadFileCallback);
        FileLoader::Get()->RegisterReadCallback({".ee", ".bl", ".nl", ".fl"}, AvalancheArchive::ReadFileCallback);
        FileLoader::Get()->RegisterReadCallback({".epe", ".blo"}, RuntimeContainer::ReadFileCallback);
        FileLoader::Get()->RegisterReadCallback(
            {".dds", ".ddsc", ".hmddsc", ".atx1", ".atx2"},
            [&](const std::filesystem::path& filename, FileBuffer data, bool external) {
                const uint8_t flags = (TextureManager::TextureCreateFlags_CreateIfNotExists
                                       | TextureManager::TextureCreateFlags_IsUIRenderable);

                // parse the compressed texture if the file was loaded from an external source
                if (external) {
                    if (filename.extension() == ".ddsc") {
                        FileBuffer out;
                        if (FileLoader::Get()->ReadAVTX(&data, &out)) {
                            TextureManager::Get()->GetTexture(filename, &out, flags);
                            return true;
                        }
                    } else if (filename.extension() == ".hmddsc" || filename.extension() == ".atx1"
                               || filename.extension() == ".atx2") {
                        FileBuffer out;
                        FileLoader::Get()->ReadHMDDSC(&data, &out);
                        TextureManager::Get()->GetTexture(filename, &out, flags);
                        return true;
                    }
                }

                TextureManager::Get()->GetTexture(filename, &data, flags);
                return true;
            });

        // register save file type callbacks
        FileLoader::Get()->RegisterSaveCallback({".rbm"}, RenderBlockModel::SaveFileCallback);
        FileLoader::Get()->RegisterSaveCallback({".ee", ".bl", ".nl", ".fl"}, AvalancheArchive::SaveFileCallback);
        FileLoader::Get()->RegisterSaveCallback({".epe", ".blo"}, RuntimeContainer::SaveFileCallback);

#if 1
        FileLoader::Get()->RegisterReadCallback({".meshc", ".hrmeshc"}, AvalancheDataFormat::FileReadCallback);
#endif

        // register file type context menu callbacks
        UI::Get()->RegisterContextMenuCallback({".rbm", ".modelc"}, RenderBlockModel::ContextMenuUI);
        UI::Get()->RegisterContextMenuCallback({".epe"}, RuntimeContainer::ContextMenuUI);

        // register importers and exporters
        ImportExportManager::Get()->Register(new import_export::Wavefront_Obj);
        ImportExportManager::Get()->Register(new import_export::DDSC);
        ImportExportManager::Get()->Register(new import_export::AvalancheArchive);

        //
        Renderer::Get()->Events().PostRender.connect([&](RenderContext_t* context) {
        // std::lock_guard<std::recursive_mutex> _lk{ RenderBlockModel::InstancesMutex };

#if 0
            ImGui::SetNextWindowBgAlpha(0.0f);
            ImGui::Begin("Model Manager", nullptr, (ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs));
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
#endif
            // draw gizmos
            for (const auto& model : RenderBlockModel::Instances) {
                model.second->DrawGizmos();
            }
        });

        //
        ShaderManager::Get()->Init();

        // run!
        Window::Get()->Run();
    }

    return 0;
}

#ifndef DEBUG
typedef BOOL(WINAPI* MINIDUMPWRITEDUMP)(HANDLE process, uint32_t process_id, HANDLE file, MINIDUMP_TYPE type,
                                        const PMINIDUMP_EXCEPTION_INFORMATION   exception_param,
                                        const PMINIDUMP_USER_STREAM_INFORMATION user_stream,
                                        const PMINIDUMP_CALLBACK_INFORMATION    callback);

LONG WINAPI UnhandledExceptionHandler(struct _EXCEPTION_POINTERS* exception_pointers)
{
    auto dbghelp   = LoadLibrary("dbghelp.dll");
    auto WriteDump = (MINIDUMPWRITEDUMP)GetProcAddress(dbghelp, "MiniDumpWriteDump");

    auto t    = std::time(nullptr);
    auto tm   = *std::localtime(&t);
    auto time = std::put_time(&tm, "%d-%m-%Y-%H-%M-%S");

    std::stringstream filename;
    filename << std::filesystem::current_path().generic_string();
    filename << "/" << VER_PRODUCTNAME_STR << "-" << VER_FILE_VERSION_STR << "-" << time << ".dmp";

    auto file = CreateFile(filename.str().c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, nullptr);

    _MINIDUMP_EXCEPTION_INFORMATION except_info;
    except_info.ThreadId          = GetCurrentThreadId();
    except_info.ExceptionPointers = exception_pointers;
    except_info.ClientPointers    = FALSE;

    const auto process    = GetCurrentProcess();
    const auto process_id = GetCurrentProcessId();

    WriteDump(process, process_id, file, MiniDumpNormal, &except_info, nullptr, nullptr);
    CloseHandle(file);

    std::string msg = "Something somewhere somehow went wrong.\n\n";
    msg.append("A dump file was created at \"" + filename.str() + "\"\n\n");
    msg.append("Please create a bug report on our GitHub and attach the dump file and we'll try and fix it.");

    MessageBox(nullptr, msg.c_str(), nullptr, MB_ICONSTOP | MB_OK);
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

#include <Windows.h>

#include <DbgHelp.h>
#include <shellapi.h>

#include <AvaFormatLib.h>

#include "settings.h"
#include "version.h"
#include "window.h"

#include "graphics/renderer.h"
#include "graphics/texture_manager.h"
#include "graphics/ui.h"

#include "game/file_loader.h"
#include "game/formats/avalanche_archive.h"
#include "game/formats/avalanche_model_format.h"
#include "game/formats/render_block_model.h"
#include "game/formats/runtime_container.h"
#include "game/irenderblock.h"
#include "game/render_block_factory.h"

#include "import_export/adf_to_xml.h"
#include "import_export/archive_table.h"
#include "import_export/avalanche_archive.h"
#include "import_export/ddsc.h"
#include "import_export/import_export_manager.h"
#include "import_export/resource_bundle.h"
#include "import_export/rtpc_to_xml.h"
#include "import_export/wavefront_obj.h"

extern bool g_IsJC4Mode         = false;
extern bool g_DrawBoundingBoxes = true;
extern bool g_ShowModelLabels   = true;

#include <argparse.h>

#ifndef DEBUG
LONG WINAPI UnhandledExceptionHandler(struct _EXCEPTION_POINTERS* exception_pointers);
#endif

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR psCmdLine, int32_t iCmdShow)
{
#ifndef DEBUG
    // generate minidumps
    SetUnhandledExceptionFilter(UnhandledExceptionHandler);
#endif

    ArgumentParser args(VER_FILE_DESCRIPTION_STR);
    args.add_argument("--jc3", "Start in Just Cause 3 Mode");
    args.add_argument("--jc4", "Start in Just Cause 4 Mode");

    // parse launch arguments
    try {
        args.parse(__argc, __argv);
    } catch (const ArgumentParser::ArgumentNotFound& e) {
        SPDLOG_ERROR(e.what());
    }

#if 0
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
#endif

    g_DrawBoundingBoxes = Settings::Get()->GetValue<bool>("draw_bounding_boxes", false);
    g_ShowModelLabels   = Settings::Get()->GetValue<bool>("show_model_labels", false);

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

    if (Window::Get()->Initialise(hInstance)) {
#ifndef DEBUG
        // check for any updates
        Window::Get()->CheckForUpdates();
#endif

        // set game mode
        if (args.get<bool>("jc3")) {
            Window::Get()->SwitchMode(GameMode_JustCause3);
        } else if (args.get<bool>("jc4")) {
            Window::Get()->SwitchMode(GameMode_JustCause4);
        } else {
            UI::Get()->ShowGameSelection();
        }

#ifdef DEBUG
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
                        assert(success);
                        const auto archive = AvalancheArchive::make(filename);
                        archive->Parse(data, [&](bool success) {
                            assert(success);
                            std::filesystem::path model =
                                "models/characters/main_characters/sheldon/sheldon_body_002.modelc";
                            AvalancheModelFormat::Load(model);
                        });
                    });
                }

                if (GetAsyncKeyState(VK_F2) & 1) {
                    std::filesystem::path filename = "editor/entities/gameobjects/main_character.ee";
                    FileLoader::Get()->ReadFile(filename, [&, filename](bool success, FileBuffer data) {
                        assert(success);
                        const auto archive = AvalancheArchive::make(filename);
                        archive->Parse(data, [&](bool success) {
                            assert(success);
                            std::filesystem::path rbm = "models/jc_characters/main_characters/rico/rico_body_lod1.rbm";
                            RenderBlockModel::Load(rbm);
                        });
                    });
                }

                if (GetAsyncKeyState(VK_F3) & 1) {
                    std::filesystem::path filename =
                        "editor/entities/jc_weapons/02_two_handed/w141_rpg_uvk_13/w141_rpg_uvk_13.ee";
                    FileLoader::Get()->ReadFile(filename, [&, filename](bool success, FileBuffer data) {
                        const auto archive = AvalancheArchive::make(filename);
                        archive->Parse(data, [&](bool success) {
                            std::filesystem::path rbm = "models/jc_weapons/02_two_handed/w141_rpg_uvk_13/"
                                                        "w141_rpg_uvk_13_base_body_lod1.rbm";
                            RenderBlockModel::Load(rbm);
                        });
                    });
                }

                if (GetAsyncKeyState(VK_F4) & 1) {
                    std::filesystem::path filename =
                        "editor/entities/jc_vehicles/01_land/v0405_car_mugello_moderncircuitracer/"
                        "v0405_car_mugello_moderncircuitracer_civilian_01.ee";
                    FileLoader::Get()->ReadFile(filename, [&, filename](bool success, FileBuffer data) {
                        const auto archive = AvalancheArchive::make(filename);
                        archive->Parse(data, [&](bool success) {
                            std::filesystem::path rbm =
                                "models/jc_vehicles/01_land/v0405_car_mugello_moderncircuitracer/"
                                "moderncircuitracer_body_lod1.rbm";
                            RenderBlockModel::Load(rbm);
                        });
                    });
                }

                if (GetAsyncKeyState(VK_F5) & 1) {
                    std::filesystem::path filename =
                        "editor/entities/jc_vehicles/02_air/v4602_plane_urga_fighterbomber/"
                        "v4602_plane_urga_fighterbomber_debug.ee";
                    FileLoader::Get()->ReadFile(filename, [&, filename](bool success, FileBuffer data) {
                        const auto archive = AvalancheArchive::make(filename);
                        archive->Parse(data, [&](bool success) {
                            std::filesystem::path epe =
                                "editor/entities/jc_vehicles/02_air/v4602_plane_urga_fighterbomber/"
                                "v4602_plane_urga_fighterbomber_debug.epe";
                            const auto rc = RuntimeContainer::get(epe.string());
                            if (rc) {
                                RenderBlockModel::LoadFromRuntimeContainer(epe, rc);
                            } else {
                                /*RuntimeContainer::Load(epe, [&, epe](std::shared_ptr<RuntimeContainer> rc) {
                                    RenderBlockModel::LoadFromRuntimeContainer(epe, rc);
                                });*/
                            }
                        });
                    });
                }

                if (GetAsyncKeyState(VK_F6) & 1) {
                    std::filesystem::path filename = "editor/entities/jc_vehicles/01_land/v0803_car_na_monstertruck/"
                                                     "v0803_car_na_monstertruck_civilian_01.ee";
                    FileLoader::Get()->ReadFile(filename, [&, filename](bool success, FileBuffer data) {
                        const auto archive = AvalancheArchive::make(filename);
                        archive->Parse(data, [&](bool success) {
                            std::filesystem::path epe = "editor/entities/jc_vehicles/01_land/v0803_car_na_monstertruck/"
                                                        "v0803_car_na_monstertruck_civilian_01.epe";
                            const auto rc = RuntimeContainer::get(epe.string());
                            if (rc) {
                                RenderBlockModel::LoadFromRuntimeContainer(epe, rc);
                            } else {
                                /*RuntimeContainer::Load(epe, [&, epe](std::shared_ptr<RuntimeContainer> rc) {
                                    RenderBlockModel::LoadFromRuntimeContainer(epe, rc);
                                });*/
                            }
                        });
                    });
                }

                if (GetAsyncKeyState(VK_F7) & 1) {
                    // std::filesystem::path filename = "editor/entities/gameobjects/main_character.epe";
                    /*RuntimeContainer::Load(filename, [&](std::shared_ptr<RuntimeContainer> rc) {
                        FileLoader::Get()->WriteRuntimeContainer(rc.get());
                    });*/

                    std::filesystem::path filename = "c:/users/aaron/desktop/main_character.epe";
                    FileLoader::Get()->ReadFile(filename, [&, filename](bool success, std::vector<uint8_t> buffer) {
                        auto rtpc = RuntimeContainer::make(filename);
                        rtpc->Parse(buffer, [rtpc](bool success) {
                            assert(success);
                            auto variant = rtpc->GetVariant(0x30a89270);
                            assert(variant);
                            assert(variant->Value<std::string>() == "grappling_device_jc2_default");
                        });
                    });
                }

                if (GetAsyncKeyState(VK_F10)) {
                    /*std::filesystem::path filename = "terrain/jc3/patches/patch_09_00_00.streampatch";
                    FileLoader::Get()->ReadFile(filename, [&, filename](bool success, FileBuffer data) {
                        if (success) {
                            const auto adf = AvalancheDataFormat::make(filename);
                            if (adf->Parse(data)) {
                                __debugbreak();
                            }
                        } else {
                            __debugbreak();
                        }
                    });*/

                    /*std::filesystem::path filename =
                        "editor/entities/jc_vehicles/01_land/v0401_car_mugello_racingsupercar/"
                        "v0401_car_mugello_racingsupercar_civilian_01.ee";
                    FileLoader::Get()->ReadFile(filename, [&, filename](bool success, FileBuffer data) {
                        if (success) {
                            auto archive = AvalancheArchive::make(filename, data);
                            while (!archive->GetStreamArchive()) {
                                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                            }

                            std::filesystem::path vmodc =
                    "models/jc_vehicles/01_land/v0401_car_mugello_racingsupercar/"
                                                          "racingsupercar_vehicle_parts.vmodc";
                            FileLoader::Get()->ReadFile(vmodc, [&, vmodc](bool success, FileBuffer data) {
                                if (success) {
                                    const auto adf = AvalancheDataFormat::make(vmodc);
                                    if (adf->Parse(data)) {
                                        __debugbreak();
                                    }
                                }
                            });
                        }
                    });*/

#if 0
                    std::filesystem::path filename = "editor/entities/characters/main_characters/rico.epe_adf";
                    FileLoader::Get()->ReadFile(filename, [&, filename](bool success, FileBuffer data) { });
#endif
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }).detach();
#endif

        // register read file type callbacks
        FileLoader::Get()->RegisterReadCallback({".lod", ".rbm"}, RenderBlockModel::ReadFileCallback);
        FileLoader::Get()->RegisterReadCallback({".modelc"}, AvalancheModelFormat::ReadFileCallback);
        FileLoader::Get()->RegisterReadCallback({".ee", ".bl", ".nl", ".fl"}, AvalancheArchive::ReadFileCallback);
        FileLoader::Get()->RegisterReadCallback({".epe", ".blo"}, RuntimeContainer::ReadFileCallback);
        FileLoader::Get()->RegisterReadCallback(
            {".dds", ".ddsc", ".hmddsc", ".atx1", ".atx2"},
            [&](const std::filesystem::path& filename, std::vector<uint8_t> data, bool external) {
                const uint8_t flags = (TextureManager::TextureCreateFlags_CreateIfNotExists
                                       | TextureManager::TextureCreateFlags_IsUIRenderable);

                // parse the compressed texture if the file was loaded from an external source
                if (external) {
                    if (filename.extension() == ".ddsc") {
                        std::vector<uint8_t> out_buffer;
                        if (FileLoader::Get()->ReadAVTX(data, &out_buffer)) {
                            TextureManager::Get()->GetTexture(filename, out_buffer, flags);
                            return true;
                        }
                    } else if (filename.extension() == ".hmddsc" || filename.extension() == ".atx1"
                               || filename.extension() == ".atx2") {
                        std::vector<uint8_t> out_buffer;
                        FileLoader::Get()->ParseTextureSource(&data, &out_buffer);
                        TextureManager::Get()->GetTexture(filename, out_buffer, flags);
                        return true;
                    }
                }

                TextureManager::Get()->GetTexture(filename, data, flags);
                return true;
            });

        // register save file type callbacks
        FileLoader::Get()->RegisterSaveCallback({".rbm"}, RenderBlockModel::SaveFileCallback);
        FileLoader::Get()->RegisterSaveCallback({".modelc"}, AvalancheModelFormat::SaveFileCallback);
        FileLoader::Get()->RegisterSaveCallback({".ee", ".bl", ".nl", ".fl"}, AvalancheArchive::SaveFileCallback);
        FileLoader::Get()->RegisterSaveCallback({".epe", ".blo"}, RuntimeContainer::SaveFileCallback);

        // register file type context menu callbacks
        UI::Get()->RegisterContextMenuCallback({".rbm"}, RenderBlockModel::ContextMenuUI);
        // UI::Get()->RegisterContextMenuCallback({".modelc"}, AvalancheModelFormat::ContextMenuUI);
        // UI::Get()->RegisterContextMenuCallback({".epe"}, RuntimeContainer::ContextMenuUI);

        // register importers and exporters
        ImportExportManager::Get()->Register<ImportExport::ADF2XML>();
        ImportExportManager::Get()->Register<ImportExport::ArchiveTable>();
        ImportExportManager::Get()->Register<ImportExport::AvalancheArchive>();
        ImportExportManager::Get()->Register<ImportExport::DDSC>();
        ImportExportManager::Get()->Register<ImportExport::ResourceBundle>();
        // ImportExportManager::Get()->Register<ImportExport::RTPC2XML>();
        ImportExportManager::Get()->Register<ImportExport::Wavefront_Obj>();

        // draw gizmos
        // @TODO: move to UI.
        Renderer::Get()->Events().PostRender.connect([&](RenderContext_t* context) {
            static auto white = glm::vec4{1, 1, 1, 0.7};
            static auto black = glm::vec4{0, 0, 0, 0.8};
            static auto red   = glm::vec4{1, 0, 0, 1};

            if (g_IsJC4Mode) {
                for (const auto& model : AvalancheModelFormat::Instances) {
                    // bounding boxes
                    if (g_DrawBoundingBoxes) {
                        UI::Get()->DrawBoundingBox(model.second->GetBoundingBox(), red);
                    }
                }
            } else {
                for (const auto& model : RenderBlockModel::Instances) {
                    const auto& bounding_box = model.second->GetBoundingBox();

                    // model labels
                    if (g_ShowModelLabels) {
                        UI::Get()->DrawText(model.second->GetFileName(), bounding_box.GetCenter(), white, black, true);
                    }

                    // bounding boxes
                    if (g_DrawBoundingBoxes) {
                        UI::Get()->DrawBoundingBox(bounding_box, red);

                        // auto mouse_pos = Input::Get()->GetMouseWorldPosition();

                        // Ray   ray(mouse_pos, {0, 0, 1});
                        // float distance = 0.0f;
                        // auto  intersects = m_BoundingBox.Intersect(ray, &distance);

                        // DebugRenderer::Get()->DrawBBox(m_BoundingBox.GetMin(), m_BoundingBox.GetMax(),
                        // intersects ? green : red);
                    }
                }
            }
        });

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

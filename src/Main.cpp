#include <Window.h>
#include <Input.h>
#include <Settings.h>
#include <graphics/Renderer.h>
#include <graphics/UI.h>

#include <vector>
#include <shellapi.h>

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

#include <jc3/ModelManager.h>

// normal: 56523.6641
// colour: 262143.984

extern fs::path g_JC3Directory = "";
extern bool g_DrawBoundingBoxes = true;
extern bool g_ShowModelLabels = true;
extern AvalancheArchive* g_CurrentLoadedArchive;

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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR psCmdLine, int32_t iCmdShow)
{
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

            // if the directory exists, save the settings now
            if (fs::exists(g_JC3Directory)) {
                Settings::Get()->SetValue("jc3_directory", g_JC3Directory.string());
            }
        }
    }

    if (Window::Get()->Initialise(hInstance)) {
        // do we need to select the install directory manually?
        if (g_JC3Directory.empty() || !fs::exists(g_JC3Directory)) {
            // ask the user to select their jc3 directory, we can't find it!
            Window::Get()->ShowFolderSelection("Please select your Just Cause 3 folder.", [&](const std::string& selected) {
                Settings::Get()->SetValue("jc3_directory", selected);
                g_JC3Directory = selected;
            }, [] {
                Window::Get()->ShowMessageBox("Unable to find Just Cause 3 root directory.\n\nSome features will be disabled.");
            });
        }

#ifndef DEBUG
        // check for any updates
        CheckForUpdates();
#endif

#if 1
        Input::Get()->Events().KeyUp.connect([](uint32_t key) {
            if (key == VK_F1) {
                //FileLoader::Get()->LocateFileInDictionary("models/jc_environments/props/animation_prop/textures/bucket_dif.hmddsc");
                FileLoader::Get()->LocateFileInDictionary("bucket_dif.hmddsc");
            }
        });
#endif

        // empty the texture manager once all models are unloaded
        ModelManager::Get()->Events().EmptyModels.connect([] {
            TextureManager::Get()->Empty();
        });

        // register file type callbacks now
        FileLoader::Get()->RegisterCallback(".rbm", RenderBlockModel::FileReadCallback);
        FileLoader::Get()->RegisterCallback({ ".ee", ".bl", ".nl" }, AvalancheArchive::FileReadCallback);

        // register importers and exporters
        ImportExportManager::Get()->Register(new import_export::Wavefront_Obj);
        ImportExportManager::Get()->Register(new import_export::DDSC);
        ImportExportManager::Get()->Register(new import_export::AvalancheArchive);

#if 0
        Window::Get()->Events().FileDropped.connect([](const fs::path& filename) {
            if (filename.extension() == ".rbm") {
                std::ifstream input(filename, std::ios::binary | std::ios::ate);
                auto length = input.tellg();
                input.seekg(0);

                auto model = new RenderBlockModel(filename);
                model->Parse(input, [&](bool success) {
                    if (!success) {
                        delete model;
                    }
                });

                input.close();
            }
        });
#endif

        // run!
        Window::Get()->Run();
    }

    return 0;
}
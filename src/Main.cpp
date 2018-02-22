#include <Window.h>
#include <Input.h>
#include <Settings.h>
#include <graphics/Renderer.h>
#include <graphics/UI.h>

#include <vector>
#include <json.hpp>

#include <jc3/FileLoader.h>
#include <jc3/formats/RenderBlockModel.h>
#include <jc3/formats/AvalancheArchive.h>
#include <jc3/formats/RuntimeContainer.h>

#include <import_export/ImportExportManager.h>
#include <import_export/wavefront_obj.h>

// normal: 56523.6641
// colour: 262143.984

extern fs::path g_JC3Directory = "";
extern bool g_DrawBoundingBoxes = true;
extern bool g_DrawDebugInfo = true;
extern AvalancheArchive* g_CurrentLoadedArchive;

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
    g_DrawDebugInfo = Settings::Get()->GetValue<bool>("draw_debug_info", true);

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

#if 0
        Input::Get()->Events().KeyUp.connect([](uint32_t key) {
            if (key == VK_F1) {
                // "editor/entities/jc_vehicles/01_land/v0012_car_autostraad_atv/v0012_car_autostraad_atv_civilian_01.ee"
                FileLoader::Get()->ReadFileFromArchive("game35", 0x6DC24513, [](bool success, FileBuffer data) {
                    auto ee = new AvalancheArchive("v0012_car_autostraad_atv_civilian_01.ee", data);

                    auto [archive, entry] = FileLoader::Get()->GetStreamArchiveFromFile("v0012_car_autostraad_atv_civilian_01.epe");
                    if (archive && entry.m_Offset != 0) {
                        auto epe_data = archive->GetEntryFromArchive(entry);
                        auto runtime_container = FileLoader::Get()->ReadRuntimeContainer(epe_data);

                        if (runtime_container) {
                            auto props = runtime_container->GetContainer(0xE9146D12);

                            // TODO: getting this property doesn't seem to work.
                            // looks like the first child container overwrites it???
                            auto body_name = props->GetProperty(0xD31AB684)->GetValue();
                            DEBUG_LOG(std::any_cast<std::string>(body_name));
 
                            for (auto& children : props->GetContainers()) {
                                auto child_name = children->GetProperty(0xD31AB684)->GetValue();
                                DEBUG_LOG(std::any_cast<std::string>(child_name));
                            }
                        }
                    }
                });
            }
            else if (key == VK_F2) {
                fs::path filename = "v1400_boat_mugello_spyspeedboat_civilian_01.ee";
                FileLoader::Get()->ReadFile(filename, [filename](bool success, FileBuffer data) {
                    auto ee = new AvalancheArchive(filename, data);

                    auto [archive, entry] = FileLoader::Get()->GetStreamArchiveFromFile("spyspeedboat_interior_lod1.rbm");
                    if (archive && entry.m_Offset != 0) {
                        auto rbm_data = archive->GetEntryFromArchive(entry);
                        std::istringstream stream(std::string{ (char*)rbm_data.data(), rbm_data.size() });

                        auto rbm = new RenderBlockModel(filename);
                        rbm->ParseRenderBlockModel(stream);

                        auto obj_exporter = new Wavefront_Obj;
                        obj_exporter->Export(rbm);
                    }
                });
            }
        });
#endif

        // Register file type callbacks now
        FileLoader::Get()->RegisterCallback(".rbm", RenderBlockModel::FileReadCallback);
        FileLoader::Get()->RegisterCallback({ ".ee", ".bl", ".nl" }, AvalancheArchive::FileReadCallback);

#if 0
        // Register importers and exporters
        ImportExportManager::Get()->Register(new Wavefront_Obj);
#endif

        Window::Get()->Run();
    }

    return 0;
}
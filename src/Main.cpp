#include <Window.h>
#include <Input.h>
#include <Settings.h>
#include <graphics/Renderer.h>
#include <graphics/UI.h>

#include <vector>
#include <json.hpp>

#include <jc3/FileLoader.h>
#include <jc3/formats/RenderBlockModel.h>
#include <jc3/formats/ExportedEntity.h>

#include <shlobj.h>

// normal: 32512.4961
// colour: 262143.984

extern fs::path g_JC3Directory = "";
extern bool g_DrawBoundingBoxes = true;
extern bool g_DrawDebugInfo = true;

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
            std::thread th([] {
                TCHAR szDir[MAX_PATH];

                BROWSEINFO bi;
                ZeroMemory(&bi, sizeof(bi));

                bi.hwndOwner = Window::Get()->GetHwnd();
                bi.pidlRoot = nullptr;
                bi.pszDisplayName = szDir;
                bi.lpszTitle = "Please select your Just Cause 3 folder.";
                bi.ulFlags = BIF_RETURNONLYFSDIRS;
                bi.lpfn = nullptr;

                LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
                if (pidl) {
                    SHGetPathFromIDList(pidl, szDir);
                    g_JC3Directory = szDir;

                    DEBUG_LOG("selected: " << g_JC3Directory);
                    Settings::Get()->SetValue("jc3_directory", g_JC3Directory.string());
                }
            });

            th.detach();
        }

        Input::Get()->Events().KeyUp.connect([](uint32_t key) {
            if (key == VK_F1) {
                // "editor/entities/jc_vehicles/01_land/v0012_car_autostraad_atv/v0012_car_autostraad_atv_civilian_01.ee"

                FileLoader::Get()->ReadFileFromArchive("game35", 0x6DC24513, [](bool success, std::vector<uint8_t> data) {
                    auto ee = new ExportedEntity("v0012_car_autostraad_atv_civilian_01.ee", data);

                    auto [archive, entry] = FileLoader::Get()->GetStreamArchiveFromFile("v0012_car_autostraad_atv_civilian_01.epe");
                    if (archive && entry.m_Offset != 0) {
                        auto epe_data = archive->ReadEntryFromArchive(entry);
                        FileLoader::Get()->ReadRuntimeContainer(epe_data);
                    }
                });
            }
        });

        UI::Get()->Events().FileTreeItemSelected.connect([](const std::string& filename) {
            DEBUG_LOG("look for " << filename);

            // try find the file in our loaded archives first
            auto [archive, entry] = FileLoader::Get()->GetStreamArchiveFromFile(filename);
            if (archive && entry.m_Offset != 0) {
                auto data = archive->ReadEntryFromArchive(entry);
                new RenderBlockModel(filename, data);

                DEBUG_LOG("Found " << filename << " in already loaded stream archive");

                return true;
            }

            FileLoader::Get()->LocateFileInDictionary(filename, [](bool success, std::string archive, uint32_t namehash, std::string filename) {
                if (success) {
                    DEBUG_LOG(archive);
                    DEBUG_LOG(namehash << " " << filename);

                    FileLoader::Get()->ReadFileFromArchive(archive, namehash, [filename](bool success, std::vector<uint8_t> data) {
                        if (success) {
                            DEBUG_LOG("Read from archive finished.");

                            // TODO: a proper handler for this stuff
                            if (filename.find(".ee") != std::string::npos ||
                                filename.find(".bl") != std::string::npos ||
                                filename.find(".nl") != std::string::npos) {
                                new ExportedEntity(filename, data);
                            }
                            else if (filename.find(".rbm") != std::string::npos) {
                                new RenderBlockModel(filename, data);
                            }
                        }
                    });
                }
                else {
                    DEBUG_LOG("[WARNING] FileTreeItemSelected -> LocateFileInDictionary - Can't find " << filename << " in dictionary");
                }
            });

            return true;
        });
    
        Window::Get()->Run();
    }

    return 0;
}
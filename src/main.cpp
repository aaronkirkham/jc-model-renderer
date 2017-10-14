#include <Window.h>
#include <Input.h>
#include <graphics/Renderer.h>
#include <graphics/UI.h>

#include <vector>
#include <json.hpp>

#include <jc3/FileLoader.h>
#include <jc3/formats/RenderBlockModel.h>
#include <jc3/formats/ExportedEntity.h>

// normal: 32512.4961
// colour: 262143.984

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR psCmdLine, int32_t iCmdShow)
{
#if 0
    float packed = 32512.4961f;

    auto r = UnpackVector(262143.984f, true);

    while (true) { Sleep(100); };
    return false;
#endif

#if 0
    int16_t x = 0xff61;
    int16_t y = 0xd1df;
    int16_t z = 0x8e83;

    auto _x = JustCause3::Vertex::unpack(x);
    auto _y = JustCause3::Vertex::unpack(y);
    auto _z = JustCause3::Vertex::unpack(z);

    auto _x1 = JustCause3::Vertex::pack<int16_t>(-0.004852f);
    auto _y1 = JustCause3::Vertex::pack<int16_t>(-0.360393f);
    auto _z1 = JustCause3::Vertex::pack<int16_t>(-0.886654f);

    while (true) { Sleep(100); };
    return false;
#endif

    if (Window::Get()->Initialise(hInstance)) {
        Input::Get()->Events().KeyUp.connect([](uint32_t key) {
            if (key == VK_F1) {
                //JustCause3::ArchiveTable::VfsArchive archive;
                //FileLoader::Get()->ReadArchiveTable("D:/Steam/steamapps/common/Just Cause 3/archives_win64/game0.tab", &archive);

                //auto model = new RenderBlockModel("E:/jc3-unpack/editor/entities/gameobjects/main_character_unpack/models/jc_characters/main_characters/rico/rico_body_lod3.rbm");
                //auto model = new RenderBlockModel("E:/jc3-unpack/models/jc_characters/animals/cow/cow_branded_mesh_body_lod1.rbm");

                auto model = new RenderBlockModel("E:/jc3-unpack/locations/rom/region_01/air_races/ar_r1_02_unpack/models/jc_design_tools/racing_arrows/general_green_outter_body_lod1.rbm");
                model->SetPosition(model->GetPosition() + glm::vec3{ -2, 0, 0 });

                auto model1 = new RenderBlockModel("E:/jc3-unpack/locations/rom/region_01/air_races/ar_r1_02_unpack/models/jc_design_tools/racing_arrows/general_red_outter_body_lod1.rbm");

                auto model2 = new RenderBlockModel("E:/jc3-unpack/locations/rom/region_01/air_races/ar_r1_02_unpack/models/jc_design_tools/racing_arrows/general_white_outter_body_lod1.rbm");
                model2->SetPosition(model2->GetPosition() + glm::vec3{ 2, 0, 0 });

                auto model3 = new RenderBlockModel("E:/jc3-unpack/locations/rom/region_01/air_races/ar_r1_02_unpack/models/jc_design_tools/racing_arrows/races_teal_arrow_body_lod1.rbm");
                model3->SetPosition(model3->GetPosition() + glm::vec3{ 0, 0, 3 });
            }
            else if (key == VK_F2) {
                //new RenderBlockModel("E:/jc3-unpack/models/jc_characters/animals/cow/cow_branded_mesh_body_lod1.rbm");
                //new RenderBlockModel("E:/jc3-unpack/models/jc_characters/animals/deer/deer_buck_mesh_body_lod1.rbm");

                //new ExportedEntity("E:/jc3-unpack/editor/entities/jc_characters/animals/cow/cow.ee");
                new ExportedEntity("E:/jc3-unpack/editor/entities/gameobjects/main_character.ee");
            }
            else if (key == VK_F3) {
                auto model = new RenderBlockModel("E:/jc3-unpack/models/jc_characters/animals/seagull/seagull_body_lod1.rbm");
                model->SetScale(glm::vec3{ 2, 2, 2 });
            }
            else if (key == VK_F4) {
                new RenderBlockModel("E:/jc3-unpack/editor/entities/gameobjects/main_character_unpack/models/jc_characters/main_characters/rico/rico_body_lod3.rbm");
                //m_Models.emplace_back(new RenderBlockModel("E:/jc3-unpack/models/jc_characters/main_characters/rico/rico_body_lod1.rbm"));
            }
        });

        UI::Get()->Events().FileTreeItemSelected.connect([](const std::string& filename) {
            DEBUG_LOG("look for " << filename);

            FileLoader::Get()->LocateFileInDictionary(filename, [](bool success, std::string archive, uint32_t namehash, std::string filename) {
                if (success) {
                    DEBUG_LOG(archive);
                    DEBUG_LOG(namehash << " " << filename);

                    FileLoader::Get()->ReadFileFromArchive(archive, filename, namehash);
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
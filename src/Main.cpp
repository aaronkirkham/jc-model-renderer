#include <Window.h>
#include <Input.h>
#include <graphics/Renderer.h>

#include <vector>
#include <json.hpp>

#include <jc3/FileLoader.h>
#include <jc3/formats/RenderBlockModel.h>

std::vector<RenderBlockModel*> m_Models;

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

                m_Models.emplace_back(model);
                m_Models.emplace_back(model1);
                m_Models.emplace_back(model2);
                m_Models.emplace_back(model3);
            }
            else if (key == VK_F2) {
                //m_Models.emplace_back(new RenderBlockModel("E:/jc3-unpack/models/jc_characters/animals/cow/cow_branded_mesh_body_lod1.rbm"));
                m_Models.emplace_back(new RenderBlockModel("E:/jc3-unpack/models/jc_characters/animals/deer/deer_buck_mesh_body_lod1.rbm"));
            }
            else if (key == VK_F3) {
                auto model = new RenderBlockModel("E:/jc3-unpack/models/jc_characters/animals/seagull/seagull_body_lod1.rbm");
                model->SetScale(glm::vec3{ 2, 2, 2 });

                m_Models.emplace_back(model);
            }
            else if (key == VK_F4) {
                m_Models.emplace_back(new RenderBlockModel("E:/jc3-unpack/editor/entities/gameobjects/main_character_unpack/models/jc_characters/main_characters/rico/rico_body_lod3.rbm"));
                //m_Models.emplace_back(new RenderBlockModel("E:/jc3-unpack/models/jc_characters/main_characters/rico/rico_body_lod1.rbm"));
            }
        });

        Renderer::Get()->Events().RenderFrame.connect([](RenderContext_t* context) {
            for (auto& model : m_Models) {
                model->Draw(context);
            }

            // debug model info
            ImGui::Begin("Model Stats", nullptr, ImVec2(800, 800), 0.0f, (ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove));
            {
                std::size_t vertices = 0;
                std::size_t indices = 0;
                std::size_t triangles = 0;

                for (auto& model : m_Models) {
                    for (auto& block : model->GetRenderBlocks()) {
                        auto index_count = block->GetIndexBuffer()->m_ElementCount;

                        vertices += block->GetVertexBuffer()->m_ElementCount;
                        indices += index_count;
                        triangles += (index_count / 3);
                    }
                }

                ImGui::Text("Models: %d, Vertices: %d, Indices: %d, Triangles: %d, Textures: %d", m_Models.size(), vertices, indices, triangles, TextureManager::Get()->GetCacheSize());

                for (auto& model : m_Models) {
                    size_t block_index = 0;

                    ImGui::Text("%s", model->GetPath().filename().string().c_str());

                    for (auto& block : model->GetRenderBlocks()) {
                        ImGui::Text("\t%d: %s", block_index, block->GetTypeName());
                        block_index++;
                    }
                }
            }
            ImGui::End();
        });

#if 0
        DirectoryList list;

        list.add("animations/jc_cinematics/test.ban");

        list.add("animations/jc_cinematics/act2/mm250_csb/animations/0000_mm250_csb__char_sheldon.ban");
        list.add("animations/jc_cinematics/act2/mm250_csb/animations/test.ban");

        list.add("ai/tiles/32_43.navmeshc");
        list.add("ai/tiles/34_34.navmeshc");

        DEBUG_LOG(list.GetStructure().dump().c_str());
        __debugbreak();
#endif

    
        Window::Get()->Run();
    }

    return 0;
}
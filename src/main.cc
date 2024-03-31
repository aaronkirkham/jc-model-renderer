#include "pch.h"

#include "version.h"

#include "app/app.h"
#include "app/os.h"
#include "game/game.h"

// for argparse
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <argparse.h>
#include <backends/imgui_impl_win32.h>

int main(int argc, char** argv)
{
    ImGui_ImplWin32_EnableDpiAwareness();

    ArgumentParser parser(VER_FILE_DESCRIPTION_STR);
    parser.add_argument("-g", "--game", "Game to switch to automatically when starting", false);

    try {
        parser.parse(argc, argv);
    } catch (const ArgumentParser::ArgumentNotFound& ex) {
        std::cout << ex.what() << std::endl;
        return 1;
    }

    if (parser.is_help()) {
        parser.print_help();
        return 0;
    }

    using namespace jcmr;

    EGame       startup_game = EGame::EGAME_COUNT;
    const auto& game         = parser.get<std::string>("game");
    if (game == "jc3") {
        startup_game = EGame::EGAME_JUSTCAUSE3;
    } else if (game == "jc4") {
        startup_game = EGame::EGAME_JUSTCAUSE4;
    }

    auto* app = App::create(startup_game);
    os::init(*app);
    App::destroy(app);
    return 0;
}
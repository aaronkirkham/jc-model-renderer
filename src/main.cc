#include "pch.h"

#include "app/app.h"
#include "app/os.h"

#include <backends/imgui_impl_win32.h>

int main(int argc, char** argv)
{
    ImGui_ImplWin32_EnableDpiAwareness();

    using namespace jcmr;

    auto* app = App::create();
    os::init(*app);
    App::destroy(app);
    return 1;
}
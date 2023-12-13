#include "pch.h"

void __assert_handler(const char* expr_str, const char* file, int line)
{
    fmt::print(bg(fmt::terminal_color::red) | fg(fmt::terminal_color::white), "\n!!! ASSERTION FAILED !!!");
    fmt::print(fg(fmt::terminal_color::bright_black), "\nExpression:");
    fmt::print(" {}\n", expr_str);
    fmt::print(fg(fmt::terminal_color::bright_black), "File:");
    fmt::print(" {}\n", file);
    fmt::print(fg(fmt::terminal_color::bright_black), "Line:");
    fmt::print(" {}\n", line);

    // StaticString<2046> message("Assertion Failed!\n\nExpression: ", expr_str, "\nFile: ", file, "\nLine: ", line);
    // os::showMessageBox("Fatal Error", message, os::ICON_ERROR);
}
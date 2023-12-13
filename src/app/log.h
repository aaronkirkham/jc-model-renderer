#ifndef JCMR_APP_LOG_H_HEADER_GUARD
#define JCMR_APP_LOG_H_HEADER_GUARD

#define LOG_INFO(...) jcmr::log_info(__VA_ARGS__)
#define LOG_WARNING(...) jcmr::log_warning(__VA_ARGS__)
#define LOG_ERROR(...) jcmr::log_error(__VA_ARGS__)
#define LOG_FATAL(...) jcmr::log_fatal(__VA_ARGS__)

namespace jcmr
{
template <typename... Args> void log_info(const char* format, const Args&... args)
{
    fmt::vprint(format, fmt::make_format_args(args...));
    fmt::print("\n");
}

template <typename... Args> void log_warning(const char* format, const Args&... args)
{
    fmt::print(fg(fmt::terminal_color::magenta), "WARNING: ");
    fmt::vprint(format, fmt::make_format_args(args...));
    fmt::print("\n");
}

template <typename... Args> void log_error(const char* format, const Args&... args)
{
    fmt::print(fg(fmt::terminal_color::red), "ERROR: ");
    fmt::vprint(format, fmt::make_format_args(args...));
    fmt::print("\n");
}

template <typename... Args> void log_fatal(const char* format, const Args&... args)
{
    auto str = fmt::vformat(format, fmt::make_format_args(args...));

    fmt::print(bg(fmt::terminal_color::red) | fg(fmt::terminal_color::black), "FATAL:");
    fmt::print(" {}", str);
    fmt::print("\n");

    // __fatal_handler(str.c_str());
}
} // namespace jcmr

#endif // JCMR_APP_LOG_H_HEADER_GUARD

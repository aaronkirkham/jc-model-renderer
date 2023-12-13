#include "pch.h"

#include "debug.h"

#define NOGDI
#include <windows.h>
//
#include <DbgHelp.h>
#pragma comment(lib, "DbgHelp.lib")

#include <sstream>

namespace jcmr
{
static void getStack(CONTEXT& context, std::stringstream& out)
{
    alignas(IMAGEHLP_SYMBOL64) char symbol_mem[sizeof(IMAGEHLP_SYMBOL64) + 256];
    IMAGEHLP_SYMBOL64*              symbol = (IMAGEHLP_SYMBOL64*)symbol_mem;

    alignas(IMAGEHLP_LINE64) char line_mem[sizeof(IMAGEHLP_LINE64)];
    IMAGEHLP_LINE64*              line = (IMAGEHLP_LINE64*)line_mem;

    out << "Callstack:" << std::endl;

    HANDLE  process             = GetCurrentProcess();
    HANDLE  thread              = GetCurrentThread();
    DWORD64 symbol_displacement = 0;
    DWORD   line_displacement   = 0;
    DWORD   machine_type;

    STACKFRAME64 stack;
    memset(&stack, 0, sizeof(STACKFRAME64));
#ifdef _M_X64
    machine_type           = IMAGE_FILE_MACHINE_AMD64;
    stack.AddrPC.Offset    = context.Rip;
    stack.AddrPC.Mode      = AddrModeFlat;
    stack.AddrStack.Offset = context.Rsp;
    stack.AddrStack.Mode   = AddrModeFlat;
    stack.AddrFrame.Offset = context.Rbp;
    stack.AddrFrame.Mode   = AddrModeFlat;
#else
#error not supported
#endif

    BOOL result;
    char name[256];
    do {
        result = StackWalk64(machine_type, process, thread, &stack, &context, nullptr, SymFunctionTableAccess64,
                             SymGetModuleBase64, nullptr);

        symbol->SizeOfStruct  = sizeof(IMAGEHLP_SYMBOL64);
        symbol->MaxNameLength = 255;

        BOOL  symbol_valid = SymGetSymFromAddr64(process, (ULONG64)stack.AddrPC.Offset, &symbol_displacement, symbol);
        DWORD err          = GetLastError();
        DWORD num_char     = UnDecorateSymbolName(symbol->Name, (PSTR)name, 256, UNDNAME_COMPLETE);

        SymGetLineFromAddr64(process, stack.AddrPC.Offset, &line_displacement, line);

        // append the call stack line if this came from our source
        LOG_INFO("{}", line->FileName);
        if (strstr(line->FileName, "jc-model-renderer")) {
            auto path = std::string{strstr(line->FileName, "src")};
            auto stack_line =
                fmt::format("{:x}\t{}() - {}:{}\n", symbol->Address, symbol->Name, path, line->LineNumber);
            out << stack_line;
        }

    } while (result);
}

static LONG WINAPI unhandledExceptionHandler(LPEXCEPTION_POINTERS info)
{
    auto process = GetCurrentProcess();
    SymInitialize(process, nullptr, TRUE);
    SymRefreshModuleList(process);

    struct CrashInfo {
        LPEXCEPTION_POINTERS info;
        DWORD                thread_id;
    };

    auto dumper = [](void* data) -> DWORD {
        auto info = ((CrashInfo*)data)->info;

        char minidump_path[MAX_PATH];
        GetCurrentDirectory(sizeof(minidump_path), minidump_path);
        // catString(minidump_path, "\\minidump.dmp");

        if (info) {
            // get stack
        } else {
            // message.data[0] = '\0';
        }

        auto process    = GetCurrentProcess();
        auto process_id = GetCurrentProcessId();

        // create minidump file
        auto file = CreateFile(minidump_path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        auto type = (MINIDUMP_TYPE)(MiniDumpWithFullMemoryInfo | MiniDumpFilterMemory | MiniDumpWithHandleData
                                    | MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules);

        //
        MINIDUMP_EXCEPTION_INFORMATION exception_info{};
        exception_info.ThreadId          = ((CrashInfo*)data)->thread_id;
        exception_info.ExceptionPointers = info;
        exception_info.ClientPointers    = FALSE;

        // write minidump to file
        MiniDumpWriteDump(process, process_id, file, type, (info ? &exception_info : nullptr), nullptr, nullptr);
        CloseHandle(file);
        return 0;
    };

    DWORD     thread_id;
    CrashInfo crash_info = {info, GetCurrentThreadId()};

    auto handle = CreateThread(0, 0x8000, dumper, &crash_info, 0, &thread_id);
    WaitForSingleObject(handle, INFINITE);

    // get stack

    return EXCEPTION_CONTINUE_SEARCH;
}

void installUnhandledExceptionHandler()
{
    SetUnhandledExceptionFilter(unhandledExceptionHandler);
}
} // namespace jcmr
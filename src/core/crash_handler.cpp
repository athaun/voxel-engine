#include "crash_handler.h"

#ifdef _WIN32

#include <windows.h>
#include <dbghelp.h>
#include <csignal>
#include <cstdio>
#include <exception>
#include <mutex>

#pragma comment(lib, "dbghelp.lib")

namespace {
    std::mutex g_report_mutex;

    void print_stack_from_context(CONTEXT* context) {
        HANDLE process = GetCurrentProcess();
        HANDLE thread = GetCurrentThread();

        SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
        SymInitialize(process, nullptr, TRUE);

        STACKFRAME64 frame = {};
        DWORD machine_type;

#if defined(_M_X64)
        machine_type = IMAGE_FILE_MACHINE_AMD64;
        frame.AddrPC.Offset = context->Rip;
        frame.AddrFrame.Offset = context->Rbp;
        frame.AddrStack.Offset = context->Rsp;
#else
        machine_type = IMAGE_FILE_MACHINE_I386;
        frame.AddrPC.Offset = context->Eip;
        frame.AddrFrame.Offset = context->Ebp;
        frame.AddrStack.Offset = context->Esp;
#endif
        frame.AddrPC.Mode = AddrModeFlat;
        frame.AddrFrame.Mode = AddrModeFlat;
        frame.AddrStack.Mode = AddrModeFlat;

        fprintf(stderr, "\n===== CRASH STACK TRACE (thread %lu) =====\n", GetCurrentThreadId());

        for (int i = 0; i < 64; ++i) {
            if (!StackWalk64(machine_type, process, thread, &frame, context,
                              nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr)) {
                break;
            }
            if (frame.AddrPC.Offset == 0) break;

            char symbol_buffer[sizeof(SYMBOL_INFO) + 256] = {};
            SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(symbol_buffer);
            symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            symbol->MaxNameLen = 255;

            DWORD64 displacement = 0;
            IMAGEHLP_LINE64 line = {};
            line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
            DWORD line_displacement = 0;

            if (SymFromAddr(process, frame.AddrPC.Offset, &displacement, symbol)) {
                if (SymGetLineFromAddr64(process, frame.AddrPC.Offset, &line_displacement, &line)) {
                    fprintf(stderr, "  #%d %s + 0x%llx  (%s:%lu)\n", i, symbol->Name,
                            static_cast<unsigned long long>(displacement), line.FileName, line.LineNumber);
                } else {
                    fprintf(stderr, "  #%d %s + 0x%llx\n", i, symbol->Name,
                            static_cast<unsigned long long>(displacement));
                }
            } else {
                fprintf(stderr, "  #%d 0x%016llx\n", i, static_cast<unsigned long long>(frame.AddrPC.Offset));
            }
        }

        fprintf(stderr, "===========================================\n");
        fflush(stderr);
    }

    LONG WINAPI unhandled_exception_filter(EXCEPTION_POINTERS* info) {
        std::lock_guard<std::mutex> lock(g_report_mutex);
        fprintf(stderr, "\n*** Unhandled exception 0x%08lx at %p on thread %lu ***\n",
                info->ExceptionRecord->ExceptionCode,
                info->ExceptionRecord->ExceptionAddress,
                GetCurrentThreadId());
        print_stack_from_context(info->ContextRecord);
        return EXCEPTION_EXECUTE_HANDLER;
    }

    void report_from_here(const char* label) {
        std::lock_guard<std::mutex> lock(g_report_mutex);
        fprintf(stderr, "\n*** %s on thread %lu ***\n", label, GetCurrentThreadId());

        CONTEXT context = {};
        RtlCaptureContext(&context);
        print_stack_from_context(&context);
        fflush(stderr);
    }

    void abort_handler(int) {
        report_from_here("abort() called");
        _exit(3);
    }

    void handle_terminate() {
        report_from_here("std::terminate() called");
        _exit(3);
    }
}

namespace CrashHandler {
    void install() {
        SetUnhandledExceptionFilter(unhandled_exception_filter);
        signal(SIGABRT, abort_handler);
        std::set_terminate(handle_terminate);
    }
}

#else

namespace CrashHandler {
    void install() {}
}

#endif

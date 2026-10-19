#pragma once

#include <windows.h>

#include <cstdarg>
#include <cstdio>
#include <string>

// 调试输出助手：日志会出现在 Visual Studio「输出 -> 调试」窗口。
inline void DebugLog(wchar_t const* message)
{
    OutputDebugStringW(message);
    OutputDebugStringW(L"\n");
}

inline void DebugLog(std::wstring const& message)
{
    DebugLog(message.c_str());
}

inline void DebugLogF(wchar_t const* format, ...)
{
    wchar_t buffer[1024]{};
    va_list args;
    va_start(args, format);
    vswprintf_s(buffer, _countof(buffer), format, args);
    va_end(args);
    DebugLog(buffer);
}

inline std::wstring FormatDouble(double value, int decimals)
{
    wchar_t format[16]{};
    swprintf_s(format, L"%%.%df", decimals);

    wchar_t buffer[64]{};
    swprintf_s(buffer, format, value);
    return buffer;
}

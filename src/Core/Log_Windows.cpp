#include "Log.h"

#include <cstdio>

#include <windows.h>

void Log(const char *fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    int cnt = vsnprintf(buf, 1024, fmt, args);
    //buf[cnt] = '\n';
    //buf[cnt+1] = '\0';
    va_end(args);
    OutputDebugStringA(buf);
}